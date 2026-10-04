/*
 * mujoe_cmsis.h
 *
 *  Created on: Oct 9, 2025
 *      Author: Joe
 */

#ifndef MUJOE_CMSIS_H_
#define MUJOE_CMSIS_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "cmsis_os.h"
#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef void (*mujoe_cmsis_evtHdlr_Fp_t)( void* arg);

typedef uint32_t TaskProfiler_t;

#define CMSIS_TASK_MSG_BUFF_LEN			MAX_TASK_QUEUE_MSG_LEN//32

typedef struct _mujoe_cmsis_task_msg
{
	uint8_t			id;								// msg id
	uint8_t			buf[CMSIS_TASK_MSG_BUFF_LEN];	// message buffer
	uint8_t			len;							// message length

}mujoe_cmsis_task_msg_t;

typedef struct _mujoe_cmsis_evt_timer
{
	osTimerId_t	   		timerId;
	osEventFlagsId_t	evtFlagId;
	uint32_t			flag;
}mujoe_cmsis_evt_timer_t;

typedef struct _mujoe_cmsis_cfg
{
	osThreadFunc_t 		threadFnc;
	void*				pThreadFncArgs;
	uint32_t			maxQueueMsgCount;
	uint8_t				queueEventId;

}mujoe_cmsis_cfg_t;

typedef struct _mujoe_cmsis
{
	osThreadId_t				threadId;
	osEventFlagsId_t			eventFlagId;
	osMessageQueueId_t			msgQueueId;
	mujoe_cmsis_evt_timer_t		**ppEvtTable;
	uint8_t						numEvents;
	uint8_t						queueEventId;

}mujoe_cmsis_t, *MUJOE_CMSIS_CTX;

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

// Cannot use bit 31, reserved for CMSIS error codes
#define MUJOE_CMSIS_EVTFLG_QUEUE_MSG_RXD		(uint32_t)BV(30)	// causes error

////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_cmsis_init_context( MUJOE_CMSIS_CTX ctx, mujoe_cmsis_cfg_t *pCfg, osThreadAttr_t *pThreadAttr );
extern mujoe_err_t mujoe_cmsis_threadSetPriority( MUJOE_CMSIS_CTX ctx, osPriority_t newPriority );
extern mujoe_err_t mujoe_cmsis_threadResumeSuspend( MUJOE_CMSIS_CTX ctx, bool resume );
extern mujoe_err_t mujoe_cmsis_threadTerminate( MUJOE_CMSIS_CTX ctx );
extern size_t mujoe_cmsis_getFreeHeapSize( void );

extern mujoe_cmsis_evt_timer_t* mujoe_cmsis_createEventTimer( MUJOE_CMSIS_CTX ctx, uint32_t eventFlag, bool singleShot );
extern mujoe_err_t mujoe_cmsis_createEventTable( MUJOE_CMSIS_CTX ctx, uint8_t numEvents );
extern mujoe_err_t mujoe_cmsis_setEvent( MUJOE_CMSIS_CTX ctx, uint8_t eventId, uint32_t delay_ms );

extern mujoe_err_t mujoe_cmsis_sendMessage( MUJOE_CMSIS_CTX ctx, uint8_t id, void* pMsg, size_t msgLen, uint8_t msg_prio );
extern mujoe_err_t mujoe_cmsis_sendMessage_noData( MUJOE_CMSIS_CTX ctx, uint8_t id );

// Stripped down versions of parent functions for use within ISRs
extern void mujoe_cmsis_setEvent_ISR( MUJOE_CMSIS_CTX ctx, uint8_t eventId );
extern void mujoe_cmsis_sendMessage_ISR( MUJOE_CMSIS_CTX ctx, uint8_t id, void* pMsg, size_t msgLen, uint8_t msg_prio );

#endif /* MUJOE_CMSIS_H_ */
