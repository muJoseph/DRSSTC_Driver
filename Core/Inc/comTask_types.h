/*
 * comTask_types.h
 *
 *  Created on: Oct 17, 2025
 *      Author: Joe
 */

#ifndef COMTASK_TYPES_H_
#define COMTASK_TYPES_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	COMMTASK_MSG_ID_USART1_DATA	= (uint8_t)0,
	COMMTASK_MSG_ID_TRIG_SYS_RST,
	COMMTASK_MSG_ID_USART_TX_DMA_TC,
	COMMTASK_MSG_ID_TX_CMD_RSP,
	COMMTASK_MSG_ID_RESDRV_FLAGS,
	COMMTASK_MSG_ID_ST_RES_PER_CAP_BUFF_DUMP,

}comTask_msg_id_t;

typedef struct
{

}txCmdRspMsg_t;

#endif /* COMTASK_TYPES_H_ */
