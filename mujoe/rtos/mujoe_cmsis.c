/*
 * mujoe_cmsis.c
 *
 *  Created on: Oct 9, 2025
 *      Author: Joe
 */

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_cmsis.h"
#include <string.h>


////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION
////////////////////////////////////////////////////////////////////////

static void osTimerFunc_Cb(void *argument);


////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////

#define MS_TO_TICKS(MS)				(MS)*osKernelGetTickFreq()/1000

#define MUJOE_CMSIS_MALLOC(N)			MUJOE_MALLOC((N))
#define MUJOE_CMSIS_FREE(PTR)			MUJOE_FREE((PTR))

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_cmsis_init_context( MUJOE_CMSIS_CTX ctx, mujoe_cmsis_cfg_t *pCfg, osThreadAttr_t *pThreadAttr )
{
	mujoe_err_t err = MUJOE_ERR_FAILURE;
	memset(ctx, 0, sizeof(mujoe_cmsis_t));
	ctx->queueEventId = pCfg->queueEventId;

	ctx->threadId = osThreadNew(pCfg->threadFnc,pCfg->pThreadFncArgs, pThreadAttr);
	ctx->eventFlagId = osEventFlagsNew(NULL);
	ctx->msgQueueId = osMessageQueueNew( pCfg->maxQueueMsgCount,
			sizeof(mujoe_cmsis_task_msg_t),
			NULL);

	if( ctx->threadId && ctx->eventFlagId && ctx->msgQueueId )
	{
		err = MUJOE_ERR_NONE;
	}
	else
	{
		err = MUJOE_ERR_FAILURE;
	}
	return err;
}// mujoe_cmsis_init_context

mujoe_err_t mujoe_cmsis_threadSetPriority( MUJOE_CMSIS_CTX ctx, osPriority_t newPriority )
{
	return osThreadSetPriority(ctx->threadId, newPriority) == osOK ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_cmsis_threadSetPriority

mujoe_err_t mujoe_cmsis_threadResumeSuspend( MUJOE_CMSIS_CTX ctx, bool resume )
{
	osStatus_t stat = osOK;
	if( resume )
	{
		stat = osThreadResume(ctx->threadId);
	}
	else
	{
		stat = osThreadSuspend(ctx->threadId);
	}

	return stat == osOK ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_cmsis_threadResumeSuspend

mujoe_err_t mujoe_cmsis_threadTerminate( MUJOE_CMSIS_CTX ctx )
{
	return osThreadTerminate(ctx->threadId) == osOK ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_cmsis_threadTerminate

size_t mujoe_cmsis_getFreeHeapSize( void )
{
	return xPortGetFreeHeapSize();
} // mujoe_cmsis_getFreeHeapSize


mujoe_err_t mujoe_cmsis_sendMessage_noData( MUJOE_CMSIS_CTX ctx, uint8_t id )
{
	return mujoe_cmsis_sendMessage( ctx, id, NULL, 0, 0 );
}// mujoe_cmsis_sendMessage_noData

// priority (higher numbers indicate a higher priority)
mujoe_err_t mujoe_cmsis_sendMessage( MUJOE_CMSIS_CTX ctx, uint8_t id, void* pMsg, size_t msgLen, uint8_t msg_prio )
{
	if( msgLen > CMSIS_TASK_MSG_BUFF_LEN )
	{
		return MUJOE_ERR_INV_PARAM;
	}

	mujoe_cmsis_task_msg_t msg;
	memset(&msg,0,sizeof(mujoe_cmsis_task_msg_t));
	msg.id = id;
	msg.len = msgLen;
	// Copy message into buffer
	if( pMsg != NULL && msgLen != 0 )
	{
		memcpy(msg.buf,pMsg,msgLen);
	}


	osStatus_t stat  = osMessageQueuePut (ctx->msgQueueId,
			(const void *)&msg,
			msg_prio,
			0 ) ;

	// Notify task of new message
	if( stat == osOK )
	{
		return (osEventFlagsSet(ctx->eventFlagId, BV(ctx->queueEventId) ) & osFlagsError) ?
				MUJOE_ERR_FAILURE : MUJOE_ERR_NONE;
	}
	else
	{
		if( stat == osErrorResource )
		{
			LOG_ERROR("mujoe_cmsis_sendMessage:: Not enough space on queue!");
		}
		return MUJOE_ERR_FAILURE;
	}
}// mujoe_cmsis_sendMessage


void mujoe_cmsis_setEvent_ISR( MUJOE_CMSIS_CTX ctx, uint8_t eventId )
{
	osEventFlagsSet(ctx->eventFlagId, BV(eventId) );

}// mujoe_cmsis_setEvent_ISR

mujoe_err_t mujoe_cmsis_setEvent( MUJOE_CMSIS_CTX ctx, uint8_t eventId, uint32_t delay_ms )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	if( eventId < ctx->numEvents)
	{
		mujoe_cmsis_evt_timer_t *pEvtTmr = ctx->ppEvtTable[eventId];

		// Stop timer if running
		osStatus_t stat = osOK;
		if( osTimerIsRunning(pEvtTmr->timerId) )
		{
			stat = osTimerStop(pEvtTmr->timerId);
		}

		if( stat == osOK )
		{
			if( delay_ms == 0 )
			{
				err = (osEventFlagsSet(ctx->eventFlagId, BV(eventId) ) & osFlagsError) ?
						MUJOE_ERR_FAILURE : MUJOE_ERR_NONE;
			}
			else
			{
				err = osTimerStart( pEvtTmr->timerId, MS_TO_TICKS(delay_ms) ) == osOK ?
						MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
		}
		else
			err = MUJOE_ERR_FAILURE;
	}
	else
		err = MUJOE_ERR_INV_PARAM;


	return err;

}// mujoe_cmsis_setEvent


void mujoe_cmsis_sendMessage_ISR( MUJOE_CMSIS_CTX ctx, uint8_t id, void* pMsg, size_t msgLen, uint8_t msg_prio )
{
	if( msgLen > CMSIS_TASK_MSG_BUFF_LEN )
	{
		TRAP_MCU();
	}

	mujoe_cmsis_task_msg_t msg = {0};
	msg.id = id;
	msg.len = msgLen;
	memcpy(msg.buf,pMsg,msgLen);

	osMessageQueuePut (ctx->msgQueueId, (const void *)&msg, msg_prio, 0 ) ;	// Timeout must be zero for invocation within isr
	osEventFlagsSet(ctx->eventFlagId, BV(ctx->queueEventId));

}// mujoe_cmsis_sendMessage_ISR

mujoe_err_t mujoe_cmsis_createEventTable( MUJOE_CMSIS_CTX ctx, uint8_t numEvents )
{
	mujoe_err_t err = MUJOE_ERR_FAILURE;
	if( numEvents < 32 )
	{
		ctx->numEvents = numEvents;
		//uint32_t evtTblSize = sizeof(mujoe_cmsis_evt_timer_t*)*numEvents;

		// Create event table
		ctx->ppEvtTable = (mujoe_cmsis_evt_timer_t**)MUJOE_CMSIS_MALLOC(sizeof(mujoe_cmsis_evt_timer_t*)*numEvents);

		if( ctx->ppEvtTable != NULL )
		{
			err = MUJOE_ERR_NONE;
			// Create event timers and store within event table
			for( uint8_t i = 0; i < numEvents; i++ )
			{
				mujoe_cmsis_evt_timer_t *pEvtTmr = mujoe_cmsis_createEventTimer( ctx, BV(i), true );
				if( pEvtTmr != NULL )
				{
					ctx->ppEvtTable[i] = pEvtTmr;
				}
				// Event timer creation failed, abort
				else
				{
					err = MUJOE_ERR_MALLOC_FAILED;
					break;
				}
			}

			// Malloc failed...
			if( err != MUJOE_ERR_NONE )
			{
				// Free any created event timers
				for( uint8_t i = 0; i < numEvents; i++ )
				{
					MUJOE_CMSIS_FREE(ctx->ppEvtTable[i]);
				}

				// Free event table itself
				MUJOE_CMSIS_FREE(ctx->ppEvtTable);
			}
		}
	}

	return err;

}// mujoe_cmsis_createEventTable

mujoe_cmsis_evt_timer_t* mujoe_cmsis_createEventTimer( MUJOE_CMSIS_CTX ctx, uint32_t eventFlag, bool singleShot )
{
	mujoe_cmsis_evt_timer_t* pTmr = (mujoe_cmsis_evt_timer_t*)MUJOE_CMSIS_MALLOC(sizeof(mujoe_cmsis_evt_timer_t));

	if( pTmr != NULL  )
	{
		memset(pTmr,0,sizeof(mujoe_cmsis_evt_timer_t));
		pTmr->flag = eventFlag;
		pTmr->evtFlagId = ctx->eventFlagId;
		pTmr->timerId = osTimerNew( osTimerFunc_Cb, singleShot ? osTimerOnce : osTimerPeriodic, pTmr, NULL );
		if( pTmr->timerId == NULL )
		{
			MUJOE_CMSIS_FREE(pTmr);
		}
	}

	return pTmr;
} // mujoe_cmsis_createSingleShotTimer

////////////////////////////////////////////////////////////////////////
// STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////

static void osTimerFunc_Cb(void *argument)
{
	mujoe_cmsis_evt_timer_t* pTmr = (mujoe_cmsis_evt_timer_t *)argument;
	osEventFlagsSet(pTmr->evtFlagId, pTmr->flag );
}// osTimerFunc_Cb


