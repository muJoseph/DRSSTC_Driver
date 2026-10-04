/*
 * resDriveTask.h
 *
 *  Created on: Oct 17, 2025
 *      Author: Joe
 */

#ifndef RESDRIVETASK_TYPES_H_
#define RESDRIVETASK_TYPES_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	RESDRVTASK_MSG_ID_BOOT_MGR_CHECKIN	= (uint8_t)0,
	RESDRVTASK_MSG_ID_START_BURST,
	RESDRVTASK_MSG_ID_STOP_BURST,
	RESDRVTASK_MSG_ID_SET_LEAD_TIME,
	RESDRVTASK_MSG_ID_ACK_FAULT,
	RESDRVTASK_MSG_ID_CTRL_BUZZER,
	RESDRVTASK_MSG_ID_CTRL_HVBUS,
	RESDRVTASK_MSG_ID_SET_OCD_PARAMS,
	RESDRVTASK_MSG_ID_START_BURST_SEQ_TIMED,
	RESDRVTASK_MSG_ID_CTRL_LED,

}resDriveTask_msg_id_t;


typedef struct _startBurstMsg
{
	uint32_t	startFreq_Hz;
	uint32_t	prf_period_us;
	uint32_t	prf_ontime_us;
	uint32_t	leadTime_ns;
	uint32_t	numBurstCycles;
	uint8_t		startFreq_maxNegDev_percnt;	// Starting frequency max negative deviation (%) (min tracked freq)
	uint8_t		startFreq_maxPosDev_percnt; // Starting frequency max positive deviation (%) (max tracked freq)

}startBurstMsg_t;

typedef uint16_t comp_hysterisis_t;

typedef struct _setOcdParamsMsg
{
	uint16_t 				ocdLimit_A;			// MAX OCD
	comp_hysterisis_t		inputHysteresis;
}setOcdParamsMsg_t;

typedef struct _startBurstSeqTimedMsg
{
	startBurstMsg_t		burstParams;
	uint32_t			seqDuration_ms;

}startBurstSeqTimedMsg_t;

#endif /* #ifndef RESDRIVETASK_TYPES_H_ */
