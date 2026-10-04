/*
 * mujoe_ser_comm_drsstc.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Joe
 */

#ifndef MUJOE_SER_COMM_DRSSTC_H_
#define MUJOE_SER_COMM_DRSSTC_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_ser_comm_types.h"
#include "mujoe_cmsis.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

#define DRSSTC_SOL_DATA_ID_RES_PER_CAP_CHUNK		(uint16_t)1

typedef enum
{
	DRSSTC_CMD_WHO_AM_I 				= (sc_cmd_id_t)0x0001,
	DRSSTC_CMD_RESET 					= (sc_cmd_id_t)0x0002,
	DRSSTC_CMD_START_BURST 				= (sc_cmd_id_t)0x0003,
	DRSSTC_CMD_STOP_BURST 				= (sc_cmd_id_t)0x0004,
	DRSSTC_CMD_ACK_FAULT 				= (sc_cmd_id_t)0x0005,
	DRSSTC_CMD_CTRL_HVBUS				= (sc_cmd_id_t)0x0006,
	DRSSTC_CMD_SET_LEADTIME 			= (sc_cmd_id_t)0x0007,
	DRSSTC_CMD_SET_OCD_PARAMS 			= (sc_cmd_id_t)0x0008,
	DRSSTC_CMD_START_BURST_SEQ_TIMED 	= (sc_cmd_id_t)0x0009,
	DRSSTC_CMD_GET_RES_PER_CAP_BUFFER 	= (sc_cmd_id_t)0x000A,
}mujoe_sc_drsstc_cmd_t;

typedef struct _mujoe_sc_drsstc_cfg
{
	MUJOE_CMSIS_CTX		commTaskCmsisCtx;
	MUJOE_CMSIS_CTX		resDriveTaskCmsisCtx;
}mujoe_sc_drsstc_cfg_t;

typedef struct _mujoe_sc_drsstc
{
	mujoe_sc_drsstc_cfg_t		cfg;

}mujoe_sc_drsstc_t, *MUJOE_SC_DRSSTC;

////////////////////////////////////////////////////////////////////////
// EXTERN VARIABLES
////////////////////////////////////////////////////////////////////////

extern const int num_cmd_handlers;
extern const mujoe_sc_cmd_handler_t cmd_handlers_drsstc[];

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern void mujoe_sc_drsstc_init( MUJOE_SC_DRSSTC ctx, mujoe_sc_drsstc_cfg_t *pCfg );


#endif /* MUJOE_SER_COMM_DRSSTC_H_ */
