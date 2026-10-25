/*
 * mujoe_ser_comm_drsstc.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_ser_comm_drsstc.h"
#include <string.h>
#include "mujoe_settings.h"
#include "comTask_types.h"
#include "resDriveTask_types.h"
#include "resDriveTask.h"
#include "commTask.h"
////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

void mujoe_sc_drsstc_init( MUJOE_SC_DRSSTC ctx, mujoe_sc_drsstc_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(mujoe_sc_drsstc_t));
	ctx->cfg = *pCfg;

}// mujoe_sc_drsstc_init
////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////

static bool whoAmI_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{

	LOG_INFO("whoAmI_cmdHdlr: sending system information");

	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
	pCmdRsp->paramLen = 6;
	pCmdRsp->params[0] = MUJOE_FW_VER_MAJOR;
	pCmdRsp->params[1] = MUJOE_FW_VER_MINOR;
	pCmdRsp->params[2] = MUJOE_HW_VER_MAJOR;
	pCmdRsp->params[3] = MUJOE_HW_VER_MINOR;
	pCmdRsp->params[4] = MUJOE_SC_VER_MAJOR;
	pCmdRsp->params[5] = MUJOE_SC_VER_MINOR;

	return true;

}// whoAmI_cmdHdlr

static bool startBurst_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	bool issueCmdRsp = true;

	if( cmdArgLen == sizeof(startBurstMsg_t))
	{
		err |=  mujoe_cmsis_sendMessage( ctx->cfg.resDriveTaskCmsisCtx,
										RESDRVTASK_MSG_ID_START_BURST,
										(startBurstMsg_t *)pCmdArg,
										sizeof(startBurstMsg_t),
										0 );

		if( err == MUJOE_ERR_NONE )
		{
			// Do not issue command response on success, will be sent down stream
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
			issueCmdRsp = false;
		}
		else
		{
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
		}
	}


	return issueCmdRsp;

}// startBurst_cmdHdlr

static bool stopBurst_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
	bool issueCmdRsp = true;

	if( mujoe_cmsis_sendMessage_noData(  ctx->cfg.resDriveTaskCmsisCtx, RESDRVTASK_MSG_ID_STOP_BURST ) == MUJOE_ERR_NONE )
	{
		// Do not issue command response on success, will be sent down stream
		pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
		issueCmdRsp = false;
	}

	return issueCmdRsp;
}// stopBurst_cmdHdlr

static bool setLeadTime_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	bool issueCmdRsp = true;

	if( cmdArgLen == sizeof(uint32_t))
	{
		err |=  mujoe_cmsis_sendMessage( ctx->cfg.resDriveTaskCmsisCtx,
										RESDRVTASK_MSG_ID_SET_LEAD_TIME,
										(uint32_t *)pCmdArg,
										sizeof(uint32_t),
										0 );

		if( err == MUJOE_ERR_NONE )
		{
			// Do not issue command response on success, will be sent down stream
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
			issueCmdRsp = false;
		}
		else
		{
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
		}
	}

	return issueCmdRsp;

}// setLeadTime_cmdHdlr

static bool systemReset_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	mujoe_err_t err = MUJOE_ERR_NONE;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
	pCmdRsp->paramLen = 0;
	LOG_INFO("systemReset_cmdHdlr: Performing system reset...");
	err |=  mujoe_cmsis_sendMessage_noData( ctx->cfg.commTaskCmsisCtx,
										  COMMTASK_MSG_ID_TRIG_SYS_RST );

	return true;

}// systemReset_cmdHdlr

static bool ackFault_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	resDrvTsk_notifyAckFault();
	return false;	// do not issue command response

}// ackFault_cmdHdlr

static bool controlHvBus_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	pCmdRsp->paramLen = 0;
	bool issueRsp = true;

	if( cmdArgLen == 1 )
	{
		bool powerOn = (bool)pCmdArg[0];
		// Trigger high voltage power on/off sequence
		if( resDrvTsk_control_highVoltageBus( powerOn, true ) == MUJOE_ERR_NONE  )
		{
			// Start buzzer sequence
			if( powerOn )
			{
				resDrvTsk_control_buzzer( BUZZER_SEQ_HV_BUS_ON );
			}

			// Do not issue command response immediately on success
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
			issueRsp = false;

		}
		else
		{
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
		}
	}

	// Issue immediate response on failure
	return issueRsp;

}// controlHvBus_cmdHdlr

static bool setOcdLimit_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	bool issueCmdRsp = true;

	if( cmdArgLen == sizeof(setOcdParamsMsg_t))
	{
		err |=  mujoe_cmsis_sendMessage( ctx->cfg.resDriveTaskCmsisCtx,
										RESDRVTASK_MSG_ID_SET_OCD_PARAMS,
										(setOcdParamsMsg_t *)pCmdArg,
										sizeof(setOcdParamsMsg_t),
										0 );

		if( err == MUJOE_ERR_NONE )
		{
			// Do not issue command response on success, will be sent down stream
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
			issueCmdRsp = false;
		}
		else
		{
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
		}

	}

	return issueCmdRsp;

}// setOcdLimit_cmdHdlr

static bool startBurstSeqTimed_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	MUJOE_SC_DRSSTC ctx = (MUJOE_SC_DRSSTC)pUarg;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	bool issueCmdRsp = true;

	if( cmdArgLen == sizeof(startBurstSeqTimedMsg_t))
	{
		err |=  mujoe_cmsis_sendMessage( ctx->cfg.resDriveTaskCmsisCtx,
										RESDRVTASK_MSG_ID_START_BURST_SEQ_TIMED,
										(startBurstSeqTimedMsg_t *)pCmdArg,
										sizeof(startBurstSeqTimedMsg_t),
										0 );

		if( err == MUJOE_ERR_NONE )
		{
			// Do not issue command response on success, will be sent down stream
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
			issueCmdRsp = false;
		}
		else
		{
			pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
		}
	}


	return issueCmdRsp;

}// startBurstSeqTimed_cmdHdlr

static bool readResPeriodCaptureBuffer_cmdHdlr( void *pUarg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_INV_PARAMS;
	bool issueCmdRsp = true;

	err |= comTask_start_resPerCapDump();

	if( err == MUJOE_ERR_NONE )
	{
		// Do not issue command response on success, will be sent down-stream
		pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_SUCCESS;
		issueCmdRsp = false;
	}
	else
	{
		pCmdRsp->cmd_rsp_id = SC_FRAME_ID_CMDRSP_FAILURE;
	}



	return issueCmdRsp;

}// readResPeriodCaptureBuffer_cmdHdlr

const mujoe_sc_cmd_handler_t cmd_handlers_drsstc [] =
{
	{.cmdId = DRSSTC_CMD_WHO_AM_I, .handlerCb = whoAmI_cmdHdlr},
	{.cmdId = DRSSTC_CMD_RESET, .handlerCb = systemReset_cmdHdlr},
	{.cmdId = DRSSTC_CMD_START_BURST, .handlerCb = startBurst_cmdHdlr},
	{.cmdId = DRSSTC_CMD_STOP_BURST, .handlerCb = stopBurst_cmdHdlr},
	{.cmdId = DRSSTC_CMD_ACK_FAULT, .handlerCb = ackFault_cmdHdlr},
	{.cmdId = DRSSTC_CMD_CTRL_HVBUS, .handlerCb = controlHvBus_cmdHdlr},
	{.cmdId = DRSSTC_CMD_SET_LEADTIME, .handlerCb = setLeadTime_cmdHdlr},
	{.cmdId = DRSSTC_CMD_SET_OCD_PARAMS, .handlerCb = setOcdLimit_cmdHdlr},
	{.cmdId = DRSSTC_CMD_START_BURST_SEQ_TIMED, .handlerCb = startBurstSeqTimed_cmdHdlr},
	{.cmdId = DRSSTC_CMD_GET_RES_PER_CAP_BUFFER, .handlerCb = readResPeriodCaptureBuffer_cmdHdlr},

};// cmd_handlers_drsstc

const int num_cmd_handlers = sizeof(cmd_handlers_drsstc)/sizeof(mujoe_sc_cmd_handler_t);

