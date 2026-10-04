/*
 * mujoe_bootMgr.h
 *
 *  Created on: Oct 17, 2025
 *      Author: Joe
 */

#ifndef MUJOE_BOOTMGR_H_
#define MUJOE_BOOTMGR_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_cmsis.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	BOOTMGR_SUBJECT_USART1 = (uint8_t)0,
	BOOTMGR_SUBJECT_RES_DRV,
	BOOTMGR_NUM_SUBJECTS,

}bootMgr_subject_t;

typedef void (*bootMgr_bootComplete_Fp_t)(void* hostArg, bool success, uint32_t failureMask );
typedef mujoe_err_t (*bootMgr_submitStatus_Fp_t)( void* hostArg, bootMgr_subject_t id, bool success );

typedef struct _bootMgr_cbs
{
	bootMgr_bootComplete_Fp_t 	bootCompleteCb;
	bootMgr_submitStatus_Fp_t	submitStatusCb;

}bootMgr_cbs_t;

typedef struct _bootMgrCfg
{
	void*				hostArg;
}bootMgrCfg_t;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern void mujoe_bootMgr_initDriver( bootMgrCfg_t *pCfg );
extern void mujoe_bootMgr_registerCallbacks( bootMgr_cbs_t *pCbs );

extern mujoe_err_t mujoe_bootMgr_submitBootStatus( bootMgr_subject_t subject, bool success );
extern void mujoe_bootMgr_run( bootMgr_subject_t subject, bool success );

#endif /* MUJOE_BOOTMGR_H_ */
