/*
 * mujoe_bootMgr.c
 *
 *  Created on: Oct 17, 2025
 *      Author: Joe
 */

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bootMgr.h"
#include <string.h>
////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////


typedef struct _bootMgr
{
	bootMgrCfg_t	cfg;
	uint32_t		failureMask;
	uint32_t		checkInMask;
	uint32_t		checkInCompleteMask;
	bootMgr_cbs_t	*pCbs;
	bool			cbInvoked;

}bootMgr_t, *BOOTMGR_CTX;

////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

static bootMgr_t bootMgr;
static bootMgr_t* ctx = &bootMgr;

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

void mujoe_bootMgr_initDriver( bootMgrCfg_t *pCfg )
{
	memset(ctx,0,sizeof(bootMgr_t));
	ctx->cfg = *pCfg;

	for( uint8_t i = 0; i < BOOTMGR_NUM_SUBJECTS; i++ )
	{
		ctx->checkInCompleteMask |= ((uint32_t)0x00000001) << i;
	}

}// mujoe_bootMgr_initDriver

void mujoe_bootMgr_registerCallbacks( bootMgr_cbs_t *pCbs )
{
	ctx->pCbs = pCbs;

}// mujoe_bootMgr_registerCallbacks

mujoe_err_t mujoe_bootMgr_submitBootStatus( bootMgr_subject_t subject, bool success )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	if( ctx->pCbs && ctx->pCbs->submitStatusCb )
	{
		err = ctx->pCbs->submitStatusCb(ctx->cfg.hostArg, subject, success);
	}
	else
	{
		err = MUJOE_ERR_NOT_IMPLEMENTED;
	}

	return err;

}// mujoe_bootMgr_submitBootStatus

void mujoe_bootMgr_run( bootMgr_subject_t subject, bool success )
{
	// Check-in subject status
	ctx->checkInMask |= ((uint32_t)0x00000001) << subject;
	if( !success )
	{
		ctx->failureMask |= ((uint32_t)0x00000001) << subject;
	}

	if( ctx->checkInMask == ctx->checkInCompleteMask )
	{
		if( ctx->pCbs && ctx->pCbs->bootCompleteCb && !ctx->cbInvoked )
		{
			ctx->pCbs->bootCompleteCb(ctx->cfg.hostArg, ctx->failureMask==0,ctx->failureMask);
			ctx->cbInvoked = true;
		}
	}

}// mujoe_bootMgr_run


