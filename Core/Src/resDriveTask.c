/*
 * resDriveTask.c
 *
 *  Created on: Oct 9, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "resDriveTask.h"
#include "hrtim.h"
#include <string.h>
#include "mujoe_settings.h"
#include "mujoe_bootMgr.h"
#include "resDriveTask_types.h"
#include "commTask.h"
#include "mujoe_ser_comm_drsstc.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define RESDRV_EVTFLG_QUEUE_MSG_RXD		MUJOE_CMSIS_EVTFLG_QUEUE_MSG_RXD

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	RESDRV_EVT_PROC_QUEUE = (uint8_t)0,
	RESDRV_EVT_INIT_HW,
	RESDRV_EVT_FEEDBACK_TIMEOUT,
	RESDRV_EVT_NUM_PRF_CYCLES_COMPLETE,
	RESDRV_EVT_PROC_ADC_SAMPLE,
	RESDRV_EVT_TRIG_ZCDB_ADC_SAMPLE,
	RESDRV_EVT_BUZZER_MGR,
	RESDRV_EVT_HVPWRSEQ_MGR,
	RESDRV_EVT_LED_MGR,
	RESDRV_EVT_FINITE_BURST_SEQ_END,
	RESDRV_NUM_EVTS,

}resDriveTsk_evt_id_t;


typedef struct _resDriveTaskInitArg
{
	MUJOE_CMSIS_CTX				myCmsisCtx;

}resDriveTaskInitArg_t, *RESDRIVETASK_INITARG;

typedef struct _buzzer_seq_step
{
		uint8_t			onTime_DC;	// On time, duty cyle
		uint32_t 		period_ms;
		uint16_t		numCycle;

}buzzer_seq_step_t;

typedef struct _buzzer_sequence
{
	buzzer_seq_step_t	*pSteps;
	int					 numSteps;

}buzzer_sequence_t;

typedef struct _buzzer_mgr
{
	bool					running;
	buzzer_sequence_id_t	currSeq;
	int						currCycle;
	int						currStep;

}buzzer_mgr_t;

typedef buzzer_seq_step_t led_seq_step_t;
typedef buzzer_sequence_t led_sequence_t;
//typedef buzzer_sequence_id_t led_sequence_id_t;

typedef struct _led_mgr
{
	bool					running;
	led_sequence_id_t		currSeq;
	int						currCycle;
	int						currStep;

}led_mgr_t;


typedef enum
{
	HVPWRSEQMGR_IDLE = (uint8_t)0,
	HVPWRSEQMGR_ICL_BYPASS_DISABLE,
	HVPWRSEQMGR_HVBUS_ON,
	HVPWRSEQMGR_ICL_BYPASS_ENABLE,
	HVPWRSEQMGR_HVBUS_OFF,

}hvPwrSeqMgr_state_t;

typedef struct _hvPwrSeq_mgr
{
	hvPwrSeqMgr_state_t	state;
	bool				hvBusOn;

}hvPwrSeq_mgr_t;

typedef struct _resDriveTask
{
	RESDRIVETASK_INITARG		pTaskArg;
	uint32_t					evtAwaitMask;
	resdrv_ctx_t 				resDrvCtx;
	buzzer_mgr_t				buzzer_mgr;
	led_mgr_t					led_mgr;
	hvPwrSeq_mgr_t				hvPwrSeq_mgr;

}resDriveTask_t, *RESDRIVETASK_CTX;

////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

// For use within ISRs
static mujoe_cmsis_t resDriveTask_cmsis;
static resDriveTaskInitArg_t resDriveTaskInitArg;

// Resonant period capture buffer mutex attributes
static const osMutexAttr_t _resPerCapBufMutexAttr =
{
    "resPerCapBufMtx",
    osMutexPrioInherit,   // avoid priority inversion vs. real-time driver task
    NULL, 0U
};

// Resonant Period Capture Buffer access mutex
static osMutexId_t _resPerCapBufMutexId = NULL;

// Pointer to capture buffer
static uint32_t *_pResPerCapBuf = NULL;
// Length of capture buffer, how many uint32_t
static int _resPerCapBufLen = 0;

////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIOS
////////////////////////////////////////////////////////////////////////

static void resDriveTask(void *argument);

static void procQueue_evtHdlr( void* arg );
static void initHardware_evtHdlr( void* arg );
static void feedbackTimeout_evtHdlr( void* arg );
static void numPrfCyclesComplete_evtHdlr( void* arg );
static void procAdcSample_evtHdlr( void* arg );
static void trigZcdbAdcSample_evtHdlr( void* arg );
static void buzzerMgr_evtHdlr( void* arg );
static void hvPwrSeqMgr_evtHdlr( void* arg );
static void ledMgr_evtHdlr( void* arg );
static void finiteBurstSeqEnd_evtHdlr( void* arg );

static const mujoe_cmsis_evtHdlr_Fp_t eventHdlrs[RESDRV_NUM_EVTS] =
{
		procQueue_evtHdlr,						// RESDRV_EVT_PROC_QUEUE
		initHardware_evtHdlr,					// RESDRV_EVT_INIT_HW
		feedbackTimeout_evtHdlr,				// RESDRV_EVT_FEEDBACK_TIMEOUT
		numPrfCyclesComplete_evtHdlr,			// RESDRV_EVT_NUM_PRF_CYCLES_COMPLETE
		procAdcSample_evtHdlr,					// RESDRV_EVT_PROC_ADC_SAMPLE
		trigZcdbAdcSample_evtHdlr,				// RESDRV_EVT_TRIG_ZCDB_ADC_SAMPLE
		buzzerMgr_evtHdlr,						// RESDRV_EVT_BUZZER_MGR
		hvPwrSeqMgr_evtHdlr,					// RESDRV_EVT_HVPWRSEQ_MGR
		ledMgr_evtHdlr,							// RESDRV_EVT_LED_MGR
		finiteBurstSeqEnd_evtHdlr,				// RESDRV_EVT_FINITE_BURST_SEQ_END
};// eventHdlrs


static void bootMgr_bootComplete_CB(void* hostArg, bool success, uint32_t failureMask );
static mujoe_err_t bootMgr_submitStatus_CB(void* hostArg, bootMgr_subject_t id, bool success );

static const bootMgr_cbs_t bootMgr_cbs =
{
	.bootCompleteCb = bootMgr_bootComplete_CB,
	.submitStatusCb = bootMgr_submitStatus_CB,

};// bootMgr_cbs

static void resdrv_faultChange_CB(void* hostArg, resdrv_fault_flag_t fault );
static void resdrv_numPrfCyclesComplete_CB(void* hostArg );

static const resdrv_cbs_t resdrv_cbs =
{
		.faultChangeCB 				= resdrv_faultChange_CB,
		.numPrfCyclesCompleteCB 	= resdrv_numPrfCyclesComplete_CB,
};// resdrv_cbs

// Boot sucess
static const buzzer_seq_step_t buzzer_seq1[] =
{
		// 1 short beep
		{.onTime_DC = 50, .period_ms = 200, .numCycle = 1},
};// buzzer_seq1

// HV Bus ON
static const buzzer_seq_step_t buzzer_seq2[] =
{
		{.onTime_DC = 100, .period_ms = 2000, .numCycle = 1},
};// buzzer_seq2

// Start burst
static const buzzer_seq_step_t buzzer_seq3[] =
{
		{.onTime_DC = 50, .period_ms = 250, .numCycle = 3},
};// buzzer_seq3


#define MORSE_CODE_DOT_DURATION_MS			100
#define MORSE_CODE_DASH_DURATION_MS			MORSE_CODE_DOT_DURATION_MS * 3

// Morse code '0'
static const buzzer_seq_step_t buzzer_seq4[] =
{
		{.onTime_DC = 75, .period_ms = MORSE_CODE_DOT_DURATION_MS << 2, .numCycle = 5},	// 5 dash
};// buzzer_seq4

// Morse code '1'
static const buzzer_seq_step_t buzzer_seq5[] =
{
		{.onTime_DC = 50, .period_ms = MORSE_CODE_DOT_DURATION_MS << 1, .numCycle = 1},		// 1 dot
		{.onTime_DC = 75, .period_ms = MORSE_CODE_DOT_DURATION_MS << 2, .numCycle = 4},	// 4 dash
};// buzzer_seq5

static const buzzer_sequence_t buzzerSeqTable[BUZZER_NUM_SEQ] =
{
		{.pSteps = (buzzer_seq_step_t *)buzzer_seq1, .numSteps = sizeof(buzzer_seq1)/sizeof(buzzer_seq_step_t)},
		{.pSteps = (buzzer_seq_step_t *)buzzer_seq2, .numSteps = sizeof(buzzer_seq2)/sizeof(buzzer_seq_step_t)},
		{.pSteps = (buzzer_seq_step_t *)buzzer_seq3, .numSteps = sizeof(buzzer_seq3)/sizeof(buzzer_seq_step_t)},
		{.pSteps = (buzzer_seq_step_t *)buzzer_seq4, .numSteps = sizeof(buzzer_seq4)/sizeof(buzzer_seq_step_t)},
		{.pSteps = (buzzer_seq_step_t *)buzzer_seq5, .numSteps = sizeof(buzzer_seq5)/sizeof(buzzer_seq_step_t)},
};// buzzerSeqTable


static const led_seq_step_t led_seq1[] =
{
		{.onTime_DC = 50, .period_ms = 500, .numCycle = 10},
};// led_seq1

static const led_sequence_t ledSeqTable[LED_NUM_SEQ] =
{
		{.pSteps = (led_seq_step_t *)led_seq1, .numSteps = sizeof(led_seq1)/sizeof(led_seq_step_t)},
};// ledSeqTable


////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

mujoe_err_t resDrvTsk_init( void )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	memset(&resDriveTaskInitArg,0,sizeof(resDriveTaskInitArg_t));

	osThreadAttr_t osThreadAttr =
	{
			.name = "resDriveTask",
			.stack_size = RESDRIVE_TASK_STACK_SIZE,
			.priority = RESDRIVE_TASK_PRIORITY,
	};

	mujoe_cmsis_cfg_t cfg =
	{
			.pThreadFncArgs		= &resDriveTaskInitArg,
			.threadFnc 			= resDriveTask,
			.maxQueueMsgCount 	= RESDRIVE_TASK_QUEUE_MAX_MSGS,
			.queueEventId = RESDRV_EVT_PROC_QUEUE,
	};
	err |= mujoe_cmsis_init_context( &resDriveTask_cmsis, &cfg, &osThreadAttr );

	// Copy RTOS struct pointers into task input arg
	resDriveTaskInitArg.myCmsisCtx = &resDriveTask_cmsis;

	return err;
}// resDrvTsk_init

MUJOE_CMSIS_CTX resDrvTsk_getCmsisCtxHdl( void )
{
	return &resDriveTask_cmsis;
}// resDrvTsk_getCmsisCtxHdl

void resDrvTsk_notifyFeedbackTimeout( void )
{
	mujoe_cmsis_setEvent_ISR( &resDriveTask_cmsis, RESDRV_EVT_FEEDBACK_TIMEOUT );
}// resDrvTsk_notifyFeedbackTimeout

void resDrvTsk_notifyNumPrfCyclesComplete( void )
{
	mujoe_cmsis_setEvent_ISR( &resDriveTask_cmsis, RESDRV_EVT_NUM_PRF_CYCLES_COMPLETE );
}// resDrvTsk_notifyNumPrfCyclesComplete

mujoe_err_t resDrvTsk_notifyAckFault( void )
{
	return mujoe_cmsis_sendMessage_noData( &resDriveTask_cmsis, RESDRVTASK_MSG_ID_ACK_FAULT );

}// resDrvTsk_notifyAckFault

mujoe_err_t resDrvTsk_control_buzzer( buzzer_sequence_id_t seqId )
{
#ifdef BUZZER_EN
	uint8_t id = seqId;
	return mujoe_cmsis_sendMessage( &resDriveTask_cmsis,
								    RESDRVTASK_MSG_ID_CTRL_BUZZER, &id, 1, 0 );
#else
	return MUJOE_ERR_NONE;
#endif // #ifdef BUZZER_EN

}// resDrvTsk_control_buzzer

mujoe_err_t resDrvTsk_control_led( led_sequence_id_t seqId )
{
	uint8_t id = seqId;
	return mujoe_cmsis_sendMessage( &resDriveTask_cmsis,
									RESDRVTASK_MSG_ID_CTRL_LED, &id, 1, 0 );
}// resDrvTsk_control_led

mujoe_err_t resDrvTsk_control_highVoltageBus( bool powerOn, bool issueCmdRsp )
{
	uint8_t data[2] = {powerOn, issueCmdRsp };
	return mujoe_cmsis_sendMessage( &resDriveTask_cmsis,
									RESDRVTASK_MSG_ID_CTRL_HVBUS, &data, 2, 0 );

}// resDrvTsk_control_buzzer


// Returns the size of the buffer in terms of number of elements and number of bytes
void resDrvTsk_get_resPerCapBufferLen_bytes( int *pLen, int *pLen_bytes )
{
	*pLen = _resPerCapBufLen;
	*pLen_bytes = _resPerCapBufLen * sizeof(uint32_t);

}// resDrvTsk_get_resPerCapBufferLen

// If clearAfterCopy is true, buffer is cleared after copy
mujoe_err_t resDrvTsk_copyResPerCapBuffer( void *pDst, int dstSize, int *pOutBytes, bool clearAfterCopy )
{
    // TODO: Add logic to ensure that DMA transfer is NOT ongoing
    if( _pResPerCapBuf == NULL || _resPerCapBufLen == 0 || _resPerCapBufMutexId == NULL )
    {
    	return MUJOE_ERR_NOT_SUPPORTED;
    }

    // Abort if destination capacity is insufficient
    uint32_t nBytes = (uint32_t)_resPerCapBufLen * sizeof(uint32_t);
    if( nBytes > dstSize )
    {
    	return MUJOE_ERR_INV_PARAM;
    }

    // Acquire mutex
    if( osMutexAcquire( _resPerCapBufMutexId, osWaitForever ) != osOK )
    {
    	return MUJOE_ERR_FAILURE;
    }

    // Copy contents from buffer
    memcpy( pDst, _pResPerCapBuf, nBytes );
    *pOutBytes = nBytes;

    if( clearAfterCopy )
    {
    	memset(_pResPerCapBuf, 0, nBytes );
    }

    // Release mutex
    return osMutexRelease( _resPerCapBufMutexId ) == osOK ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// resDrvTsk_copyResPerCapBuffer


////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////

static void bootMgr_bootComplete_CB(void *hostArg, bool success, uint32_t failureMask )
{
	if( success )
	{
		MUJOE_CMSIS_CTX cmsis = (MUJOE_CMSIS_CTX)hostArg;
		mujoe_cmsis_setEvent( cmsis,
							  RESDRV_EVT_PROC_ADC_SAMPLE,
							  1000 );

		mujoe_cmsis_setEvent( cmsis,
							  RESDRV_EVT_TRIG_ZCDB_ADC_SAMPLE,
							  500 );

		// Turn on LED
		mujoe_bsp_ctrl_LED( true );

		// Enable GDT Drivers
		// TODO: Move GDT driver board enable signal downstream so system
		// is safed for majority of boot/idle period
		mujoe_bsp_ctrl_GDT_EN( true );

		if( resDrvTsk_control_buzzer( BUZZER_SEQ_BOOT_SUCCESS ) != MUJOE_ERR_NONE )
		{
			TRAP_MCU();
		}
	}
	else
	{
		TRAP_MCU();
	}
}// bootMgr_bootComplete_CB

static mujoe_err_t bootMgr_submitStatus_CB(void* hostArg, bootMgr_subject_t id, bool success )
{
	 uint8_t msg[2] = { id, success};
	 return mujoe_cmsis_sendMessage( (MUJOE_CMSIS_CTX)hostArg,
			 	 	 	 	 	 	 RESDRVTASK_MSG_ID_BOOT_MGR_CHECKIN, msg, 2, 0 );
}// bootMgr_submitStatus_CB


static void resdrv_faultChange_CB(void* hostArg, resdrv_fault_flag_t fault )
{
	comTask_submitResDriveFlags(fault);
}// resdrv_faultChange_CB

static void resdrv_numPrfCyclesComplete_CB(void* hostArg )
{
	resDrvTsk_notifyNumPrfCyclesComplete();
}// resdrv_numPrfCyclesComplete_CB

////////////////////////////////////////////////////////////////////////
// TASK HANDLER
////////////////////////////////////////////////////////////////////////

static mujoe_err_t submitCmdRsp( sc_cmd_id_t cmd, sc_cmd_rsp_id_t cmd_rsp )
{
	mujoe_sc_cmd_rsp_t cmdRsp;
	memset(&cmdRsp,0,sizeof(mujoe_sc_cmd_rsp_t));
	cmdRsp.cmd_id = cmd;
	cmdRsp.cmd_rsp_id = cmd_rsp;
	return comTask_submitCommandResponse( &cmdRsp );
}// submitCmdRsp


static bool validateStartBurstParams( startBurstMsg_t *pParams )
{

	// Check start frequency
	if( (pParams->startFreq_Hz < START_FREQ_MIN_HZ) || (pParams->startFreq_Hz > START_FREQ_MAX_HZ) )
	{
		return false;
	}

	// Check PRF Period/frequency
	uint32_t prfFreq_Hz = (float)1.0f/((float)pParams->prf_period_us * (float)1e-6);
	if( (prfFreq_Hz < PRF_FREQ_MIN_HZ) || (prfFreq_Hz > PRF_FREQ_MAX_HZ) )
	{
		return false;
	}

	// Check PRF On time
	if( (pParams->prf_ontime_us < PRF_ON_TIME_MIN_USEC) || (pParams->prf_ontime_us > PRF_ON_TIME_MAX_USEC) )
	{
		return false;
	}

	// Check PRF duty cycle
	float dc = (float)pParams->prf_ontime_us/(float)pParams->prf_period_us;
	if( dc > PRF_MAX_DUTY_CYCLE )
	{
		return false;
	}

	// Check lead time
	if( (pParams->leadTime_ns < LEAD_TIME_MIN_NSEC) || (pParams->leadTime_ns > LEAD_TIME_MAX_NSEC) )
	{
		return false;
	}


	return true;
}// validateStartBurstParams

static sc_cmd_rsp_id_t handleStartBurst( RESDRIVETASK_CTX ctx, startBurstMsg_t *pArg )
{

	sc_cmd_rsp_id_t cmdRsp = SC_FRAME_ID_CMDRSP_INV_PARAMS;

	if( validateStartBurstParams( pArg ) )
	{
		cmdRsp = SC_FRAME_ID_CMDRSP_FAILURE;

		LOG_INFO("handleStartBurst:: Burst params:\n f_start: %u Hz\n T_prf: %u us\n t_on: %u us\n t_lead: %u ns\n n_cycles: %u\n Starting burst...",
				(unsigned int)pArg->startFreq_Hz,
				(unsigned int)pArg->prf_period_us,
				(unsigned int)pArg->prf_ontime_us,
				(unsigned int)pArg->leadTime_ns,
				(unsigned int)pArg->numBurstCycles);

		// Set lead time
		if( mujoe_resdrv_set_lead_time(&ctx->resDrvCtx, pArg->leadTime_ns ) == MUJOE_ERR_NONE )
		{

			mujoe_resdrv_restoreHbridgeOutputRouting(&ctx->resDrvCtx);

			// Parameters valid, arm and start burst
			if( mujoe_resdrv_arm(&ctx->resDrvCtx) == MUJOE_ERR_NONE )
			{

				mujoe_resdrv_start_burst_arg_t arg =
				{
					.startFreq_Hz = pArg->startFreq_Hz,
					.startFreq_maxNegDev_percnt = pArg->startFreq_maxNegDev_percnt,
					.startFreq_maxPosDev_percnt = pArg->startFreq_maxPosDev_percnt,
					.period_us = pArg->prf_period_us,
					.ontime_us = pArg->prf_ontime_us,
					.numBurstCycles = pArg->numBurstCycles,
				};

				if( mujoe_resdrv_start_burst( &ctx->resDrvCtx, &arg ) == MUJOE_ERR_NONE )
				{
					// Trigger buzzer sequence
					resDrvTsk_control_buzzer( BUZZER_SEQ_MORSE_1 );

					// Trigger LED sequence
					resDrvTsk_control_led( LED_SEQ_START_BURST );

					cmdRsp = SC_FRAME_ID_CMDRSP_SUCCESS;
				}
				// Start burst failed, return to disarmed state
				else
				{
					// TEST
					mujoe_resdrv_stop_burst(&ctx->resDrvCtx);	// Probably not neccessary, but added to be safe
					mujoe_resdrv_disarm( &ctx->resDrvCtx );
				}
			}
		}

	}

	if( cmdRsp == SC_FRAME_ID_CMDRSP_SUCCESS )
	{
		LOG_INFO("handleStartBurst:: Burst started!");
	}
	else
	{
		LOG_ERROR("handleStartBurst:: Start burst FAILED!");
	}

	return cmdRsp;

} // handleStartBurst


static mujoe_err_t handleStopBurst( RESDRIVETASK_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	err |= mujoe_resdrv_stop_burst( &ctx->resDrvCtx );
	err |= mujoe_resdrv_disarm( &ctx->resDrvCtx );

	if( err == MUJOE_ERR_NONE )
	{
		resDrvTsk_control_buzzer( BUZZER_SEQ_MORSE_0 );
	}

	return err;

}// handleStopBurst


static void procQueue_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	uint32_t numMsgs = osMessageQueueGetCount( ctx->pTaskArg->myCmsisCtx->msgQueueId);
	mujoe_cmsis_task_msg_t msg = {0};

	//LOG_INFO("resDriveTask:: Processing %d message(s)", (int)numMsgs);

	// Consume all pending messages
	while( numMsgs-- > 0 )
	{
		osStatus_t stat = osMessageQueueGet( ctx->pTaskArg->myCmsisCtx->msgQueueId,&msg,NULL,0);
		if( stat == osOK )
		{
			switch(msg.id)
			{
				case RESDRVTASK_MSG_ID_BOOT_MGR_CHECKIN:
				{
					if( msg.len == 2)
					{
						mujoe_bootMgr_run((bootMgr_subject_t)msg.buf[0], (bool)msg.buf[1]);
					}
					break;
				}
				case RESDRVTASK_MSG_ID_START_BURST:
				{
					if( msg.len == sizeof(startBurstMsg_t))
					{
						startBurstMsg_t *pArg = (startBurstMsg_t *)msg.buf;
						sc_cmd_rsp_id_t cmdRsp = SC_FRAME_ID_CMDRSP_INV_PARAMS;

						cmdRsp = handleStartBurst(ctx, pArg);

						// Send command response via message queue to commTask for TX to host
						submitCmdRsp( DRSSTC_CMD_START_BURST, cmdRsp );
					}
					break;
				}
				case RESDRVTASK_MSG_ID_STOP_BURST:
				{
					mujoe_err_t err = handleStopBurst(ctx);
					submitCmdRsp( DRSSTC_CMD_STOP_BURST,  err == MUJOE_ERR_NONE ? SC_FRAME_ID_CMDRSP_SUCCESS : SC_FRAME_ID_CMDRSP_FAILURE );
					break;
				}
				case RESDRVTASK_MSG_ID_SET_OCD_PARAMS:
				{
					if( msg.len == sizeof(setOcdParamsMsg_t))
					{
						setOcdParamsMsg_t *pArg = (setOcdParamsMsg_t *)msg.buf;
						sc_cmd_rsp_id_t cmdRsp = SC_FRAME_ID_CMDRSP_INV_PARAMS;
						if( pArg->ocdLimit_A >= OCD_LIMIT_MIN_A && pArg->ocdLimit_A <= OCD_LIMIT_MAX_A )
						{
							mujoe_err_t err = MUJOE_ERR_NONE;
							err |= mujoe_resdrv_set_OCD_limit( &ctx->resDrvCtx, pArg->ocdLimit_A );
							err |= mujoe_resdrv_set_OCD_comp_input_hysterisis(&ctx->resDrvCtx, (mujoe_comp_in_hysteresis_t)pArg->inputHysteresis);

							cmdRsp = (err == MUJOE_ERR_NONE) ?  SC_FRAME_ID_CMDRSP_SUCCESS : SC_FRAME_ID_CMDRSP_FAILURE;

							if( err == MUJOE_ERR_NONE )
							{
								LOG_INFO("procQueue_evtHdlr:: OCD Params set successfully: OCD limit: %d A, Input Hysteresis: %d", pArg->ocdLimit_A, pArg->inputHysteresis);
							}
							else
							{
								LOG_ERROR("procQueue_evtHdlr:: OCD params set FAILED!");
							}


						}

						submitCmdRsp( DRSSTC_CMD_SET_OCD_PARAMS, cmdRsp );
					}
					break;
				}
				case RESDRVTASK_MSG_ID_START_BURST_SEQ_TIMED:
				{
					if( msg.len == sizeof(startBurstSeqTimedMsg_t))
					{
						startBurstSeqTimedMsg_t *pArg = (startBurstSeqTimedMsg_t *)msg.buf;
						sc_cmd_rsp_id_t cmdRsp = SC_FRAME_ID_CMDRSP_INV_PARAMS;

						if( pArg->seqDuration_ms <= BURST_SEQUENCE_MAX_DURATION_MS )
						{
							pArg->burstParams.numBurstCycles = 0;	// Ensure num burst cycles is 0 (infinite)
							cmdRsp = handleStartBurst(ctx, &pArg->burstParams);

							if( cmdRsp == SC_FRAME_ID_CMDRSP_SUCCESS )
							{
								LOG_INFO("procQueue_evtHdlr:: Halting burst sequence after %u ms delay.", (unsigned int)pArg->seqDuration_ms );

								// Set delayed event for halting burst sequence
								if( mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
														  RESDRV_EVT_FINITE_BURST_SEQ_END,
														  pArg->seqDuration_ms ) != MUJOE_ERR_NONE )
								{
									LOG_ERROR("procQueue_evtHdlr:: Failed to start delayed halt burst sequence event, stopping immediately...");
									if( handleStopBurst( ctx ) == MUJOE_ERR_NONE )
									{
										LOG_INFO("procQueue_evtHdlr:: Burst sequence stopped successfully.");
									}
									else
									{
										// TODO: Replace with assert
										LOG_ERROR("procQueue_evtHdlr:: Burst sequence failed to stop!");
									}

								}

							}

						}

						// Send command response via message queue to commTask for TX to host
						submitCmdRsp( DRSSTC_CMD_START_BURST_SEQ_TIMED, cmdRsp );
					}
					break;
				}
				case RESDRVTASK_MSG_ID_SET_LEAD_TIME:
				{
					mujoe_err_t err = MUJOE_ERR_NONE;
					uint32_t leadTime_ns = *((uint32_t *)msg.buf);
					err |= mujoe_resdrv_set_lead_time(&ctx->resDrvCtx, leadTime_ns);

					if( err == MUJOE_ERR_NONE )
					{
						LOG_INFO("procQueue_evtHdlr:: Lead time set successfully: Lead time %d ns", (int)leadTime_ns );
					}
					else
					{
						LOG_ERROR("procQueue_evtHdlr:: Lead time set FAILED!");
					}

					submitCmdRsp( DRSSTC_CMD_SET_LEADTIME, err == MUJOE_ERR_NONE ? SC_FRAME_ID_CMDRSP_SUCCESS : SC_FRAME_ID_CMDRSP_FAILURE  );
					break;
				}
				case RESDRVTASK_MSG_ID_ACK_FAULT:
				{
					mujoe_err_t err = MUJOE_ERR_NONE;
					err |= mujoe_resdrv_ackFaults( &ctx->resDrvCtx );
					err |= submitCmdRsp( DRSSTC_CMD_ACK_FAULT,
										(err == MUJOE_ERR_NONE) ? SC_FRAME_ID_CMDRSP_SUCCESS : SC_FRAME_ID_CMDRSP_FAILURE );
					break;
				}
				case RESDRVTASK_MSG_ID_CTRL_BUZZER:
					if( msg.len == 1)
					{
						buzzer_sequence_id_t id = msg.buf[0];

						if( !ctx->buzzer_mgr.running )
						{
							mujoe_bsp_ctrl_BUZZER( false );
							ctx->buzzer_mgr.running = true;
							ctx->buzzer_mgr.currSeq = id;
							mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx, RESDRV_EVT_BUZZER_MGR, 0 );
						}
					}
					break;
				case RESDRVTASK_MSG_ID_CTRL_LED:
					if( msg.len == 1)
					{
						led_sequence_id_t id = msg.buf[0];

						if( !ctx->led_mgr.running )
						{
							mujoe_bsp_ctrl_LED( false );
							ctx->led_mgr.running = true;
							ctx->led_mgr.currSeq = id;
							mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx, RESDRV_EVT_LED_MGR, 0 );
						}
					}
					break;
				case RESDRVTASK_MSG_ID_CTRL_HVBUS:
				{
					if( msg.len == 2)
					{
						mujoe_err_t err = MUJOE_ERR_FAILURE;
						bool pwrOn = msg.buf[0];
						bool issueCmdRsp = msg.buf[1];
						hvPwrSeq_mgr_t *pMgr = &ctx->hvPwrSeq_mgr;
						if( pMgr->state == HVPWRSEQMGR_IDLE )
						{
							pMgr->state = pwrOn ? HVPWRSEQMGR_ICL_BYPASS_DISABLE : HVPWRSEQMGR_HVBUS_OFF;
							err = mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx, RESDRV_EVT_HVPWRSEQ_MGR, 0 );
						}

						if( issueCmdRsp )
						{
							err |= submitCmdRsp( DRSSTC_CMD_CTRL_HVBUS,
												(err == MUJOE_ERR_NONE) ? SC_FRAME_ID_CMDRSP_SUCCESS : SC_FRAME_ID_CMDRSP_FAILURE );
						}

					}
					break;
				}
				default:
					LOG_ERROR("resDriveTask: Unexpected message ID: %d", msg.id);
					// Should not go here!
					break;
			}
		}
	}

}// procQueue_evtHdlr

static void initHardware_evtHdlr( void* arg )
{
	LOG_INFO("resDriveTask:: Initializing hardware...");

	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;

	err |= mujoe_resdrv_init_hardware( &ctx->resDrvCtx );

	err |= mujoe_bootMgr_submitBootStatus(BOOTMGR_SUBJECT_RES_DRV, err == MUJOE_ERR_NONE );

	if( err == MUJOE_ERR_NONE )
	{
		// Store res period capture buffer details in local context
		mujoe_resdrv_getResPeriodCapBuffer( &ctx->resDrvCtx, &_pResPerCapBuf, &_resPerCapBufLen );

		LOG_INFO("resDriveTask:: Init hardware complete");
	}
	else
	{
		LOG_ERROR("resDriveTask:: Init hardware failed!");
		TRAP_MCU();
	}

}// initHardware_evtHdlr

static void feedbackTimeout_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;

	LOG_INFO("feedbackTimeout_evtHdlr:: Current feedback timed out. Halting operation...");

	err |= mujoe_resdrv_handleFeedbackTimeout( &ctx->resDrvCtx );

	if( err == MUJOE_ERR_NONE )
	{
		LOG_INFO("feedbackTimeout_evtHdlr:: Current feedback timeout handled successfully!");
	}
	else
	{
		LOG_ERROR("feedbackTimeout_evtHdlr:: Current feedback timeout FAILURE!");
	}

}// feedbackTimeout_evtHdlr

static void numPrfCyclesComplete_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;


#if RES_DRV_DBG_EN == 1
	LOG_INFO("numPrfCyclesComplete_evtHdlr:: Number of PRF cycle(s) complete. Handling clean-up...");
#endif // #if RES_DRV_DBG_EN == 1

	err |= mujoe_resdrv_handleNumPrfCyclesComplete( &ctx->resDrvCtx );

	if( err == MUJOE_ERR_NONE )
	{
		// TODO: Remove after debug
		//comTask_start_resPerCapDump();

#if RES_DRV_DBG_EN == 1
		LOG_INFO("numPrfCyclesComplete_evtHdlr:: Clean-up success!");
#endif // #if RES_DRV_DBG_EN == 1
	}
	else
	{
#if RES_DRV_DBG_EN == 1
		LOG_ERROR("numPrfCyclesComplete_evtHdlr:: Clean-up FAILED!");
#endif // #if RES_DRV_DBG_EN == 1

	}


}// numPrfCyclesComplete_evtHdlr


static void procAdcSample_evtHdlr( void* arg )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;

	uint32_t vdiff_mv = mujoe_adc_codeToVdiff_mv(&ctx->resDrvCtx.adc2, ctx->resDrvCtx.rt.cs_adc_sample );

#ifdef ADC_LOG_INFO_ENABLED
	LOG_INFO("procAdcSample_evtHdlr: Current ADC Sample: %d", (int)ctx->resDrvCtx.rt.cs_adc_sample);
	LOG_INFO("procAdcSample_evtHdlr: Current Vdiff (mV): %d", (int)vdiff_mv);
#endif // ADC_LOG_INFO_ENABLED

	err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
								 RESDRV_EVT_PROC_ADC_SAMPLE,
								 1000 );
}// procAdcSample_evtHdlr

static void trigZcdbAdcSample_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;
	err |= mujoe_adc_startInjectedConversion( &ctx->resDrvCtx.adc2 );

	err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
								 RESDRV_EVT_TRIG_ZCDB_ADC_SAMPLE,
								 500 );
}// trigZcdbAdcSample_evtHdlr



static void buzzerMgr_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	//LOG_INFO("resDriveTask::buzzer_evtHdlr invoked!");
	buzzer_mgr_t *pMgr = &ctx->buzzer_mgr;

	mujoe_err_t err = MUJOE_ERR_NONE;


	if( pMgr->running )
	{
		buzzer_sequence_t *pSeq = (buzzer_sequence_t *)&buzzerSeqTable[pMgr->currSeq];

		uint16_t numCycle = pSeq->pSteps[pMgr->currStep].numCycle;
		float onTime_DC = (pSeq->pSteps[pMgr->currStep].onTime_DC)/100.0f;
		float period_ms = pSeq->pSteps[pMgr->currStep].period_ms;
		uint32_t onTime_ms = onTime_DC * period_ms;
		uint32_t offTime_ms = (1.0f-onTime_DC) * period_ms;

		uint32_t delay_ms = 0;
		bool endOfCycle = false;

		// If buzzer on, turn off
		if( mujoe_bsp_is_BUZZER_ON() )
		{
			mujoe_bsp_ctrl_BUZZER( false );
			delay_ms = offTime_ms;
			endOfCycle = true;
		}
		// If buzzer off, turn on
		else
		{
			mujoe_bsp_ctrl_BUZZER( true );
			delay_ms = onTime_ms;
		}

		err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
									 RESDRV_EVT_BUZZER_MGR,
									 delay_ms );

		// Check if we are at the end of current cycle
		if( endOfCycle )
		{
			if( ++pMgr->currCycle == numCycle )
			{
				 pMgr->currCycle = 0;
				 if( ++pMgr->currStep == pSeq->numSteps )
				 {
					 pMgr->currStep = 0;
					 pMgr->running = false;
				 }
			}
		}


	}

}// buzzerMgr_evtHdlr

static void ledMgr_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	led_mgr_t *pMgr = &ctx->led_mgr;
	mujoe_err_t err = MUJOE_ERR_NONE;

	if( pMgr->running )
	{
		led_sequence_t *pSeq = (led_sequence_t *)&ledSeqTable[pMgr->currSeq];
		float onTime_DC = (pSeq->pSteps[pMgr->currStep].onTime_DC)/100.0f;
		float period_ms = pSeq->pSteps[pMgr->currStep].period_ms;
		uint32_t delay_ms = 0;
		bool endOfCycle = false;

		// If LED on, turn off
		if( mujoe_bsp_is_LED_ON() )
		{
			mujoe_bsp_ctrl_LED( false );
			delay_ms = (1.0f-onTime_DC) * period_ms;	// compute off time
			endOfCycle = true;
		}
		// If LED off, turn on
		else
		{
			mujoe_bsp_ctrl_LED( true );
			delay_ms = onTime_DC * period_ms;		// compute on time
		}

		err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
								     RESDRV_EVT_LED_MGR,
									 delay_ms );

		// Check if we are at the end of current cycle
		if( endOfCycle )
		{
			if( ++pMgr->currCycle == pSeq->pSteps[pMgr->currStep].numCycle )
			{
				 pMgr->currCycle = 0;
				 if( ++pMgr->currStep == pSeq->numSteps )
				 {
					 pMgr->currStep = 0;
					 pMgr->running = false;
				 }
			}
		}


	}

}// ledMgr_evtHdlr

static void finiteBurstSeqEnd_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;
	LOG_INFO("finiteBurstSeqEnd_evtHdlr:: Burst sequence duration complete, halting operation....");
	err = handleStopBurst( ctx );

	if( err == MUJOE_ERR_NONE )
	{
		LOG_INFO("finiteBurstSeqEnd_evtHdlr:: Burst sequence halted successfully!");
	}
	else
	{
		LOG_ERROR("finiteBurstSeqEnd_evtHdlr:: Burst sequence failed to halt!");
	}
}// finiteBurstSeqEnd_evtHdlr


#define ICL_RES_OHMS				20.0f		// ICL Resistance (Ohms)
#define HVBUS_CAPACITANCE_FARAD		3600.0e-6	// Equivalent bus capacitance (Farad)
// HV Bus time constant, tau (sec)
#define HVBUS_TAU					(float)ICL_RES_OHMS * (float)HVBUS_CAPACITANCE_FARAD

// Allow capacitor bank to charge for 5 * tau seconds
#define HVBUS_ON_TO_ICL_BYPASS_DELAY_MS			(uint32_t)ceil((HVBUS_TAU * 5.0f) * 1000.0f)

#define HV_PWR_SEQ_MGR_RETRY_DELAY_MS			10

static void hvPwrSeqMgr_evtHdlr( void* arg )
{
	RESDRIVETASK_CTX ctx = (RESDRIVETASK_CTX)arg;
	hvPwrSeq_mgr_t *pMgr = &ctx->hvPwrSeq_mgr;
	int32_t delay_ms = -1;
	mujoe_err_t err = MUJOE_ERR_NONE;


	switch( pMgr->state )
	{
		case HVPWRSEQMGR_ICL_BYPASS_DISABLE:
			// Disable ICL bypass
			if( mujoe_bsp_ctrl_ICL_BYPASS( false ) )
			{
				pMgr->state = HVPWRSEQMGR_HVBUS_ON;
				delay_ms = 100;
			}
			else
			{
				delay_ms = HV_PWR_SEQ_MGR_RETRY_DELAY_MS;
			}
			break;
		case HVPWRSEQMGR_HVBUS_ON:
			// Power on HV bus
			if( mujoe_bsp_powerOnHighVoltage() )
			{
				pMgr->state = HVPWRSEQMGR_ICL_BYPASS_ENABLE;
				delay_ms = HVBUS_ON_TO_ICL_BYPASS_DELAY_MS;
			}
			else
			{
				delay_ms = HV_PWR_SEQ_MGR_RETRY_DELAY_MS;
			}
			break;
		case HVPWRSEQMGR_ICL_BYPASS_ENABLE:
			// Enable ICL bypass
			if( mujoe_bsp_ctrl_ICL_BYPASS( true ) )
			{
				pMgr->hvBusOn = mujoe_bsp_is_HV_ON();
				pMgr->state = HVPWRSEQMGR_IDLE;
			}
			else
			{
				delay_ms = HV_PWR_SEQ_MGR_RETRY_DELAY_MS;
			}
			break;
		case HVPWRSEQMGR_HVBUS_OFF:
			// Power off HV bus
			if( mujoe_bsp_powerOffHighVoltage() )
			{
				pMgr->hvBusOn = mujoe_bsp_is_HV_ON();
				pMgr->state = HVPWRSEQMGR_IDLE;
			}
			else
			{
				delay_ms = HV_PWR_SEQ_MGR_RETRY_DELAY_MS;
			}
			break;
		default:
			break;
	}


	if( delay_ms != -1 )
	{
		err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
									 RESDRV_EVT_HVPWRSEQ_MGR,
									 delay_ms );
	}


}// hvPwrSeqMgr_evtHdlr

// Called once at task start
static mujoe_err_t resDriveTask_init( RESDRIVETASK_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	// Create event table
	err |= mujoe_cmsis_createEventTable( ctx->pTaskArg->myCmsisCtx, RESDRV_NUM_EVTS );


	// Create mutex for accessing resonant period capture buffer between tasks
	_resPerCapBufMutexId = osMutexNew(&_resPerCapBufMutexAttr);
	if( _resPerCapBufMutexId == NULL )
	{
		TRAP_MCU();
	}


	// Create event mask
	ctx->evtAwaitMask = 0;
	for( uint8_t i = 0; i < RESDRV_NUM_EVTS; i++ )
	{
		ctx->evtAwaitMask |= BV(i);
	}


	// Init Boot Manager
	{
		bootMgrCfg_t cfg =
		{
			.hostArg = ctx->pTaskArg->myCmsisCtx,
		};
		mujoe_bootMgr_initDriver( &cfg );
		mujoe_bootMgr_registerCallbacks( (bootMgr_cbs_t *)&bootMgr_cbs );
	}

	// Init resonant driver driver
	{
		resdrv_cfg_t cfg;
		mujoe_resdrv_init_configStruct( RES_DRIVER_PRESET, &cfg );
		cfg.enableBurstReload = ENABLE_BURST_RELOAD;
		cfg.enableDMACallbacks = ENABLE_DMA_CALLBACKS;
		cfg.dbg_cfg.enable_TF2OutputPin = ENABLE_TF2_PIN_OUTPUT;
		cfg.enablePulseSkipping = ENABLE_PULSE_SKIPPING;
		cfg.leadTime_ns = HBRIDGE_LEAD_TIME_NS;
		cfg.startFreq_Hz = START_FREQ_HZ;
		mujoe_resdrv_init_context( &ctx->resDrvCtx, &hhrtim1, &cfg );

		mujoe_resdrv_registerCallbacks(&ctx->resDrvCtx, (resdrv_cbs_t *)&resdrv_cbs);
	}

	err |= mujoe_cmsis_setEvent( ctx->pTaskArg->myCmsisCtx,
								 RESDRV_EVT_INIT_HW,
								 KERNEL_START_TO_INIT_HW_DELAY_MS );

	LOG_INFO("resDriveTask:: Init complete, heap %d bytes remain", mujoe_cmsis_getFreeHeapSize());

	return err;
}// resDriveTask_init

static void resDriveTask(void *argument)
{
	resDriveTask_t resDriveTask;
	memset(&resDriveTask,0,sizeof(resDriveTask_t));
	resDriveTask.pTaskArg = (RESDRIVETASK_INITARG)argument;

	// Initialize task
	if( resDriveTask_init(&resDriveTask) != MUJOE_ERR_NONE )
	{
		TRAP_MCU();
	}

	// Terminal loop
	for(;;)
	{
		// Wait for event flags to be set
		uint32_t events = osEventFlagsWait ( resDriveTask.pTaskArg->myCmsisCtx->eventFlagId,
				resDriveTask.evtAwaitMask,
				osFlagsWaitAny,
				osWaitForever);

		// Error occurred
		if( events & osFlagsError )
		{
			LOG_ERROR("resDriveTask:: osEventFlagsWait error code: %d",(int)events );
			TRAP_MCU();
		}
		// Call event handler(s) for any pending event(s)
		else
		{
			for( uint8_t i = 0; i < RESDRV_NUM_EVTS; i++ )
			{
				if( events & BV(i) )
				{
					eventHdlrs[i](&resDriveTask);
				}
			}
		}

	}

}// resDriveTask

