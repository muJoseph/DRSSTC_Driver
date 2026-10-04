/*
 * resDriveTask.h
 *
 *  Created on: Oct 9, 2025
 *      Author: Joe
 */

#ifndef RESDRIVETASK_H_
#define RESDRIVETASK_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_cmsis.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_resdrv.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	BUZZER_SEQ_BOOT_SUCCESS = (uint8_t)0,
	BUZZER_SEQ_HV_BUS_ON,
	BUZZER_SEQ_START_BURST,
	BUZZER_SEQ_MORSE_0,
	BUZZER_SEQ_MORSE_1,
	BUZZER_NUM_SEQ,

}buzzer_sequence_id_t;

typedef enum
{
	LED_SEQ_START_BURST = (uint8_t)0,
	LED_NUM_SEQ,

}led_sequence_id_t;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t resDrvTsk_init( void );
extern MUJOE_CMSIS_CTX resDrvTsk_getCmsisCtxHdl( void );
extern void resDrvTsk_notifyFeedbackTimeout( void );
extern void resDrvTsk_notifyNumPrfCyclesComplete( void );
extern mujoe_err_t resDrvTsk_notifyAckFault( void );
extern mujoe_err_t resDrvTsk_control_buzzer( buzzer_sequence_id_t seqId );
extern mujoe_err_t resDrvTsk_control_led( led_sequence_id_t seqId );
extern mujoe_err_t resDrvTsk_control_highVoltageBus( bool powerOn, bool issueCmdRsp );

extern void resDrvTsk_get_resPerCapBufferLen_bytes( int *pLen, int *pLen_bytes );
extern mujoe_err_t resDrvTsk_copyResPerCapBuffer( void *pDst,
												int dstSize,
												int *pOutBytes,
												bool clearAfterCopy );

#endif /* INC_RESDRIVETASK_H_ */
