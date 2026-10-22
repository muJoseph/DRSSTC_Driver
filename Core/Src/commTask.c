/*
 * commTask.c
 *
 *  Created on: Oct 9, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "commTask.h"
#include <string.h>
#include "mujoe_settings.h"
#include "mujoe_bootMgr.h"
#include "cmsis_os2.h"
#include "resDriveTask.h"
////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define USART1_DMA_RX						DMA1

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	COMM_EVT_PROC_QUEUE = (uint8_t)0,
	COMM_EVT_INIT_HW,
	COMM_EVT_SYS_RST,
	COMM_EVT_GET_RES_PER_CAP_BUF,
	COMM_NUM_EVTS,

}commTsk_evt_id_t;

typedef struct _commTaskInitArg
{
	MUJOE_CMSIS_CTX				myCmsisCtx;

}commTaskInitArg_t, *COMMTASK_INITARG;


typedef enum
{
	RESCAPDUMP_STATE_GET_DATA = (uint8_t)0,
	RESCAPDUMP_STATE_TX_CHUNK,

}resPerCapDumpMgr_state_t;

typedef struct _resPerCapDumpMgr
{
	bool						running;
	uint32_t					*pBuf;
	uint8_t						*pBufHead;
	int							bufLen;
	int							bytesRemain;
	int							dataLen;		// Data length that will be TXd via ser comm
	uint16_t					chunkIdx;		// Curr chunk index that will be TXd via ser comm
	resPerCapDumpMgr_state_t	state;

}resPerCapDumpMgr_t;

typedef struct _commTask
{
	COMMTASK_INITARG			pTaskArg;
	uint32_t					evtAwaitMask;
	usart_ctx_t					usart1Ctx;
	dma_ctx_t					dma1Ctx;
	mujoe_ser_comm_t			scCtx;
	mujoe_sc_drsstc_t			scDrsstcCtx;
	uint32_t					currResDriveFlags;
	resPerCapDumpMgr_t			resPerCapDumpMgr;
}commTask_t, *COMMTASK_CTX;

////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIOS
////////////////////////////////////////////////////////////////////////

static void commTask(void *argument);

////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

static mujoe_cmsis_t commTask_cmsis;
static commTaskInitArg_t commTaskInitArg;

static uint8_t *pUsart1RxBuf = NULL;	// TODO: Implement this differently, this is sloppy and unsafe

////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////
static void procQueue_evtHdlr( void* arg );
static void initHardware_evtHdlr( void* arg );
static void sysReset_evtHdlr( void* arg );
static void getResCapBuff_evtHdlr( void* arg );

static const mujoe_cmsis_evtHdlr_Fp_t eventHdlrs[COMM_NUM_EVTS] =
{
		procQueue_evtHdlr,				// COMM_EVT_PROC_QUEUE
		initHardware_evtHdlr,			// COMM_EVT_INIT_HW
		sysReset_evtHdlr,				// COMM_EVT_SYS_RST
		getResCapBuff_evtHdlr,			// COMM_EVT_GET_RES_PER_CAP_BUF
};


static void dma1TransferComplete_CB(uint8_t dmaCh);
static void dma1TransferError_CB(uint8_t dmaCh);
static const dma_callbacks_t dma1Cbs =
{
		.transferCompleteCB = dma1TransferComplete_CB,
		.transferErrorCB = dma1TransferError_CB,

};// dma1Cbs


static mujoe_err_t mujoe_ser_comm_write_CB(void *hostArg, void *pBuf, int len );
static void mujoe_ser_comm_error_CB(void *hostArg, mujoe_sc_err_t err );

static const mujoe_ser_comm_Cbs_t mujoe_ser_comm_Cbs =
{
		.writeCb        = mujoe_ser_comm_write_CB,
		.errorCb 		= mujoe_ser_comm_error_CB,

};// mujoe_ser_comm_Cbs


static void usart1_IDLE_Cb( void );
static void usart1_RTO_Cb( void );

static const mujoe_usart_callbacks_t usart1_Cbs =
{
	.idleCb = usart1_IDLE_Cb,
	.rtoCb = usart1_RTO_Cb,
};

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

mujoe_err_t commTsk_initTask( void )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	memset(&commTaskInitArg,0,sizeof(commTaskInitArg_t));

	osThreadAttr_t osThreadAttr =
	{
			.name = "commTask",
			.stack_size = COMM_TASK_STACK_SIZE,
			.priority = COMM_TASK_PRIORITY,
	};

	mujoe_cmsis_cfg_t cfg =
	{
			.pThreadFncArgs		= &commTaskInitArg,
			.threadFnc 			= commTask,
			.maxQueueMsgCount 	= COMM_TASK_QUEUE_MAX_MSGS,
			.queueEventId = COMM_EVT_PROC_QUEUE,
	};
	err |= mujoe_cmsis_init_context( &commTask_cmsis, &cfg, &osThreadAttr );

	// Copy RTOS struct pointers into task input arg
	commTaskInitArg.myCmsisCtx = &commTask_cmsis;

	return err;
}// commTsk_init


mujoe_err_t comTask_submitCommandResponse( mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	return mujoe_cmsis_sendMessage( &commTask_cmsis,
									COMMTASK_MSG_ID_TX_CMD_RSP,
									pCmdRsp, sizeof(mujoe_sc_cmd_rsp_t), 0 );
}// comTask_submitCommandResponse

mujoe_err_t comTask_submitResDriveFlags( uint32_t flags )
{
	return mujoe_cmsis_sendMessage( &commTask_cmsis,
									COMMTASK_MSG_ID_RESDRV_FLAGS,
									&flags, sizeof(uint32_t), 0 );
}// comTask_submitResDriveFlags

mujoe_err_t comTask_start_resPerCapDump( void )
{
	return mujoe_cmsis_sendMessage_noData( &commTask_cmsis, COMMTASK_MSG_ID_ST_RES_PER_CAP_BUFF_DUMP );

}// comTask_start_resPerCapDump


mujoe_err_t comTask_enableComm( void )
{
	return MUJOE_ERR_NONE;
}// comTask_enableComm

////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////

static mujoe_err_t mujoe_ser_comm_write_CB(void *hostArg, void *pBuf, int len )
{
	return mujoe_usart_write_DMA( (USART_CTX)hostArg, pBuf, len );
}// mujoe_ser_comm_write_CB

static void mujoe_ser_comm_error_CB(void *hostArg, mujoe_sc_err_t err )
{
	LOG_ERROR("mujoe_ser_comm_error_CB:: Error: %d", (int)err );
}// mujoe_ser_comm_error_CB


static void notifyTask_usart1RxData( void )
{
	// Get number of bytes that remain to be rxd
	uint32_t bytesRemain = LL_DMA_GetDataLength(USART1_DMA_RX, USART1_DMA_RX_CH);

	// If remaining bytes equals number of bytes requested, the dma
	// controller has completed and therefore count has rolled over (circular mode)
	// TODO: This could potentially be dangerous, since a DMA transfer error could
	// occur and yield similar results. Investigate further and implement more robust solution
	uint32_t bytesRxd = (bytesRemain == USART1_DMA_RX_LEN) ?
						USART1_DMA_RX_LEN :
						USART1_DMA_RX_LEN - bytesRemain;

	mujoe_cmsis_sendMessage_ISR( &commTask_cmsis,
								 COMMTASK_MSG_ID_USART1_DATA,
								 pUsart1RxBuf,
								 bytesRxd,
								 0 );
}// notifyTask_usart1RxData

// Invoked from within ISR
static void dma1TransferComplete_CB(uint8_t dmaCh)
{
	switch( dmaCh )
	{
		case 1:
			break;
		case 2:
			break;
		case 3:
			break;
		case 6:
			notifyTask_usart1RxData();
			break;
		case 7:
		{
			bool txPending = false;
			mujoe_cmsis_sendMessage_ISR( &commTask_cmsis,
										 COMMTASK_MSG_ID_USART_TX_DMA_TC,
										 &txPending,
										 1,
										 0 );
			break;
		}
		default:
			break;
	}

}// dma1TransferComplete_CB

static void dma1TransferError_CB(uint8_t dmaCh)
{
	LOG_ERROR("DMA1 CH%d XFR error!", dmaCh);
}// dma1TransferError_CB

// Invoked from ISR
static void usart1_IDLE_Cb( void )
{
	notifyTask_usart1RxData();
	PUT_BREAKPT_HERE();

}// usart1_IDLE_Cb

static void usart1_RTO_Cb( void )
{

}// usart1_RTO_Cb

////////////////////////////////////////////////////////////////////////
// TASK HANDLER
////////////////////////////////////////////////////////////////////////

static void procQueue_evtHdlr( void* arg )
{
	COMMTASK_CTX ctx = (COMMTASK_CTX)arg;

	uint32_t numMsgs = osMessageQueueGetCount(ctx->pTaskArg->myCmsisCtx->msgQueueId);
	mujoe_cmsis_task_msg_t msg = {0};

	//LOG_INFO("commTask:: Processing %d message(s)", (int)numMsgs);

	// Consume all pending messages
	while( numMsgs-- > 0 )
	{
		osStatus_t stat = osMessageQueueGet(ctx->pTaskArg->myCmsisCtx->msgQueueId,&msg,NULL,0);
		if( stat == osOK )
		{
			switch(msg.id)
			{
				case COMMTASK_MSG_ID_USART1_DATA:
				{
					if( mujoe_ser_comm_feedParser( &ctx->scCtx, msg.buf, msg.len ) != MUJOE_ERR_NONE )
					{
						TRAP_MCU();
					}
					break;
				}
				case COMMTASK_MSG_ID_TRIG_SYS_RST:
					if( mujoe_cmsis_setEvent(ctx->pTaskArg->myCmsisCtx, COMM_EVT_SYS_RST, SYS_RESET_DELAY_MS ) != MUJOE_ERR_NONE )
					{
						TRAP_MCU();
					}
					break;
				case COMMTASK_MSG_ID_USART_TX_DMA_TC:
					if( msg.len == 1)
					{
						bool txPending = *((bool *)msg.buf);
						mujoe_usart_updateTxPending(&ctx->usart1Ctx, txPending );
					}
					break;
				case COMMTASK_MSG_ID_TX_CMD_RSP:
					if( msg.len == sizeof(mujoe_sc_cmd_rsp_t))
					{
						mujoe_ser_comm_sendCmdRspFrame( &ctx->scCtx, (mujoe_sc_cmd_rsp_t *)msg.buf );
					}
					break;
				case COMMTASK_MSG_ID_RESDRV_FLAGS:
					if( msg.len == sizeof(uint32_t))
					{
						mujoe_ser_comm_setFlags(&ctx->scCtx, *((uint32_t *)msg.buf));
					}
					break;
				case COMMTASK_MSG_ID_ST_RES_PER_CAP_BUFF_DUMP:
					// Ensure that the dump manager is not currently running
					if( !ctx->resPerCapDumpMgr.running )
					{
						// Start dump manager
						// TODO: Remove delay after debug
						if( mujoe_cmsis_setEvent(ctx->pTaskArg->myCmsisCtx,
												 COMM_EVT_GET_RES_PER_CAP_BUF,
												 MUJOE_SC_RES_PER_CAP_BUF_DMP_DELAY_MS ) != MUJOE_ERR_NONE )
						{
							TRAP_MCU();
						}
					}
					// Dump manager currently running, abort and notify via cmd rsp
					else
					{
						mujoe_sc_cmd_rsp_t cmdRsp;
						memset(&cmdRsp,0,sizeof(mujoe_sc_cmd_rsp_t));
						cmdRsp.cmd_id = DRSSTC_CMD_GET_RES_PER_CAP_BUFFER;
						cmdRsp.cmd_rsp_id = SC_FRAME_ID_CMDRSP_BUSY;
						mujoe_ser_comm_sendCmdRspFrame( &ctx->scCtx, (mujoe_sc_cmd_rsp_t *)msg.buf );
					}
					break;
				default:
					LOG_ERROR("procQueue_evtHdlr: Unexpected message ID: %d", msg.id);
					// Should not go here!
					break;
			}
		}
	}

}// procQueue_evtHdlr


static void initHardware_evtHdlr( void* arg )
{
	COMMTASK_CTX ctx = (COMMTASK_CTX)arg;
	mujoe_err_t err = MUJOE_ERR_NONE;
	LOG_INFO("commTask:: Initializing hardware...");

	err |= mujoe_usart_read_DMA( &ctx->usart1Ctx, USART1_DMA_RX_LEN );

	err |= mujoe_bootMgr_submitBootStatus(BOOTMGR_SUBJECT_USART1, err == MUJOE_ERR_NONE );

	if( err == MUJOE_ERR_NONE )
	{
		LOG_INFO("commTask:: Hardware init complete");
	}
	else
	{
		LOG_ERROR("commTask:: Hardware init failed!");
	}


}// initHardware_evtHdlr


static void sysReset_evtHdlr( void* arg )
{
	MUJOE_PERFORM_SYS_RESET();

}// sysReset_evtHdlr


#define SOL_DATA_INTER_FRAME_DELAY_MS	10
#define SOL_DATA_MGR_RETRY_DELAYS_MS	10

static void getResCapBuff_evtHdlr( void* arg )
{
	COMMTASK_CTX ctx = (COMMTASK_CTX)arg;
	resPerCapDumpMgr_t *mgr = &(((COMMTASK_CTX)arg)->resPerCapDumpMgr);

	int32_t delay_ms = -1;

	switch( mgr->state )
	{
		case RESCAPDUMP_STATE_GET_DATA:
		{
			// Ensure that the buffer is not currently allocated
			if( mgr->pBuf == NULL )
			{
				// Get the size of the mutex protected buffer
				resDrvTsk_get_resPerCapBufferLen_bytes( &mgr->bufLen, &mgr->dataLen );
				// Allocate temp buffer for copy
				mgr->pBuf = (uint32_t *)MUJOE_MALLOC(mgr->dataLen);
				if( mgr->pBuf != NULL )
				{
					int outBytes = 0;
					// Acquire mutex and copy buffer into local temp buffer
					if( resDrvTsk_copyResPerCapBuffer(mgr->pBuf, mgr->dataLen, &outBytes, true ) == MUJOE_ERR_NONE )
					{
						if( outBytes )
						{
							mgr->bytesRemain = outBytes;
							mgr->pBufHead = (uint8_t *)mgr->pBuf;
							mgr->chunkIdx = 0;
							mgr->state = RESCAPDUMP_STATE_TX_CHUNK;
							delay_ms = 0;
						}
					}
				}
				// Malloc failed
				else
				{
					TRAP_MCU();
				}
			}
			// Buffer was not previously cleared, SHOULD NOT GO HERE!!
			else
			{
				MUJOE_FREE(mgr->pBuf);
				TRAP_MCU();
			}
			break;
		}
		case RESCAPDUMP_STATE_TX_CHUNK:
		{
			if( mgr->bytesRemain > 0 )
			{
				mujoe_sc_sol_data_t sd;
				memset(&sd,0,sizeof(mujoe_sc_sol_data_t));
				sd.data_id = DRSSTC_SOL_DATA_ID_RES_PER_CAP_CHUNK;
				sd.data_len = mgr->dataLen;
				sd.chunk_idx = mgr->chunkIdx;
				sd.chunk_len = MIN( mgr->bytesRemain, SC_SOL_DATA_CHUNK_MAX_LEN );
				// Copy chunk data into payload
				memcpy(sd.chunk,mgr->pBufHead,sd.chunk_len);

				// Send frame, if successful...
				if( mujoe_ser_comm_sendSolicitedDataFrame( &ctx->scCtx, &sd ) == MUJOE_ERR_NONE )
				{
					// Advance head of temp buffer
					mgr->pBufHead += sd.chunk_len;
					// Dec bytes remaining
					mgr->bytesRemain -= sd.chunk_len;
					// Inc chunk IDX
					mgr->chunkIdx++;

					// All chunks sent
					if( mgr->bytesRemain <= 0 )
					{
						// Free buffer back to heap
						MUJOE_FREE(mgr->pBuf);
						// Reset manager in prep for next dump
						memset(mgr,0,sizeof(resPerCapDumpMgr_t));
						mgr->running = false;
					}
					// Data remains, sched next frame tx
					else
					{
						delay_ms = SOL_DATA_INTER_FRAME_DELAY_MS;
					}
				}
				// Frame TX failed, retry
				else
				{
					delay_ms = SOL_DATA_MGR_RETRY_DELAYS_MS;
				}
			}
			break;
		}
		default:
			// Should not go here!!!
			break;
	}


	if( delay_ms >=0 )
	{
		mujoe_cmsis_setEvent(ctx->pTaskArg->myCmsisCtx, COMM_EVT_GET_RES_PER_CAP_BUF, delay_ms );
	}

}// getResCapBuff_evtHdlr


// Called once at task start
static mujoe_err_t commTask_init( COMMTASK_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Create event table
	err |= mujoe_cmsis_createEventTable( ctx->pTaskArg->myCmsisCtx, COMM_NUM_EVTS );

	// Create event await mask
	ctx->evtAwaitMask = 0;
	for( uint8_t i = 0; i < COMM_NUM_EVTS; i++ )
	{
		ctx->evtAwaitMask  |= BV(i);
	}

	// Init dma instances
	{
		 dma_ctx_cfg_t cfg = {.dma_id = DMA_ID_1};
		 mujoe_dma_init_context( &ctx->dma1Ctx, &cfg );
		 mujoe_dma_registerCallbacks( &ctx->dma1Ctx, (dma_callbacks_t *)&dma1Cbs );
	}

	// Init USART
	{
		mujoe_err_t err = MUJOE_ERR_NONE;
		usart_ctx_cfg_t cfg =
		{
				.instance 					= USART1,
				.dmaCtx 					= &ctx->dma1Ctx,
				.rxDmaCh					= USART1_DMA_RX_CH,
				.txDmaCh					= USART1_DMA_TX_CH,
				.presetId 					= MUJOE_USART_PRESET_DEFAULT,
				.fifoTxThresh 				= LL_USART_FIFOTHRESHOLD_1_8,
				.fifoRxThresh 				= LL_USART_FIFOTHRESHOLD_1_8,
				.irqType 					= USART1_IRQn,
				.rxBufferLen 				= USART1_RX_BUFF_LEN,
				.txBufferLen 				= USART1_TX_BUFF_LEN,
				.enableRxHalfXfrInt 		= false,
				.enableIdleInt				= true,
				.enableRxTimeoutInt		    = false,
		};
		err |= mujoe_usart_init_context( &ctx->usart1Ctx, &cfg );

		mujoe_usart_registerCallbacks( &ctx->usart1Ctx, (mujoe_usart_callbacks_t *)&usart1_Cbs );

		// Init USART1  peripheral
		err |= mujoe_usart_initHardware( &ctx->usart1Ctx );


		// TODO: Implement this in a cleaner and safer manner
		// Copy pointer to USART1 rx buffer so that it can be used within
		// USART1 and DMA ISRs. This should be thread safe since both ISRs
		// only READ the buffer to copy its contents into an outgoing RTOS queue message
		// The commTask never accesses the USART1 driver rx buffer directly, only uses the
		// contents of the RXd queue message to feed the parser
		if( err == MUJOE_ERR_NONE )
		{
			pUsart1RxBuf = ctx->usart1Ctx.pRxBuf;
		}

	}

	// Init serial comm driver
	{

		// Init ser com DRSSTC implementation
	    mujoe_sc_drsstc_cfg_t sc_drsstc_cfg =
	    {
	    	.commTaskCmsisCtx 		= ctx->pTaskArg->myCmsisCtx,
			.resDriveTaskCmsisCtx 	= resDrvTsk_getCmsisCtxHdl(),
	    };
		mujoe_sc_drsstc_init(&ctx->scDrsstcCtx,&sc_drsstc_cfg);

		mujoe_ser_comm_cfg_t cfg =
		{
				.pHostArg = &ctx->usart1Ctx,
		};
		err |= mujoe_ser_comm_initContext( &ctx->scCtx, &cfg );
		mujoe_ser_comm_registerCmdHdlrs( &ctx->scCtx,
										 (mujoe_sc_cmd_handler_t *)&cmd_handlers_drsstc,
										 num_cmd_handlers,
										 &ctx->scDrsstcCtx );

		 mujoe_ser_comm_registerCallbacks( &ctx->scCtx, (mujoe_ser_comm_Cbs_t *)&mujoe_ser_comm_Cbs );
	}

	// Schedule hardware init event
	err |= mujoe_cmsis_setEvent(ctx->pTaskArg->myCmsisCtx, COMM_EVT_INIT_HW, KERNEL_START_TO_INIT_HW_DELAY_MS );

	return err;
}// commTask_init

static void commTask(void *argument)
{
	commTask_t commTask;
	memset(&commTask,0,sizeof(commTask_t));
	commTask.pTaskArg = (COMMTASK_INITARG)argument;

	// Initialize task
	if( commTask_init(&commTask) != MUJOE_ERR_NONE )
	{
		TRAP_MCU();
	}

	// Debug metrics
	{
		// Gets the minimum number of stack space available in words (32 bit)
		// Therefore, x4 to get value in bytes
		uint32_t getMinStackSpace_bytes = uxTaskGetStackHighWaterMark(commTask.pTaskArg->myCmsisCtx->threadId) << 2;
		size_t freeHeapSize = mujoe_cmsis_getFreeHeapSize();
		// osThreadGetStackSize and osThreadGetStackSpace not implemented for STM32 CMSIS
		//uint32_t stackSize = osThreadGetStackSize(commTask.pTaskArg->myCmsisCtx->threadId);
		//uint32_t stackSpaceSize = osThreadGetStackSpace(commTask.pTaskArg->myCmsisCtx->threadId);
		LOG_INFO("commTask:: Init complete...");
		LOG_INFO("Free heap space: %d bytes", freeHeapSize);
		LOG_INFO("Task stack size: %d bytes", (int)COMM_TASK_STACK_SIZE);
		LOG_INFO("sizeof(commTask_t): %d bytes", sizeof(commTask_t));
		LOG_INFO("Task stack min space available: %d bytes", (int)getMinStackSpace_bytes);
		//LOG_INFO("Task stack space size: %d bytes", (int)stackSpaceSize);

	}

	// Terminal loop
	for(;;)
	{
		// Wait for event flags to be set
		uint32_t events = osEventFlagsWait (commTask.pTaskArg->myCmsisCtx->eventFlagId,
				commTask.evtAwaitMask,
				osFlagsWaitAny,
				osWaitForever);

		// Error occurred
		if( events & osFlagsError )
		{
			LOG_ERROR("osEventFlagsWait error code: %d",(int)events );
			TRAP_MCU();
		}
		// Call event handler(s) for any pending event(s)
		else
		{
			for( uint8_t i = 0; i < COMM_NUM_EVTS; i++ )
			{
				if( events & BV(i) )
				{
					eventHdlrs[i](&commTask);
				}
			}
		}

	}

}// commTask

