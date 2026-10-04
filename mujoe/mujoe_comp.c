/*
 * mujoe_comp.c
 *
 *  Created on: Sep 14, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_comp.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_comp_init_context( COMP_CTX ctx, COMP_TypeDef *instance, comp_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(comp_ctx_t));
	// Store ll COMP instance
	ctx->instance = instance;
	ctx->cfg = *pCfg;

	return MUJOE_ERR_NONE;
}// mujoe_comp_init_context


mujoe_err_t mujoe_comp_activate(COMP_CTX ctx)
{

	 if( LL_COMP_IsEnabled(ctx->instance) == 1 )
		 return MUJOE_ERR_NONE;

	__IO uint32_t wait_loop_index = 0;

	/* Enable comparator */
	LL_COMP_Enable(ctx->instance);

	/* Delay for comparator startup time.                                       */
	/* Compute number of CPU cycles to wait for, from delay in us.              */
	/* Note: Variable divided by 2 to compensate partially                      */
	/*       CPU processing cycles (depends on compilation optimization).       */
	/* Note: If system core clock frequency is below 200kHz, wait time          */
	/*       is only a few CPU processing cycles.                               */
	wait_loop_index = ((LL_COMP_DELAY_STARTUP_US * (SystemCoreClock / (100000 * 2))) / 10);
	while(wait_loop_index != 0)
	{
	wait_loop_index--;
	}

	return LL_COMP_IsEnabled(ctx->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_comp_activate


mujoe_err_t mujoe_comp_set_input_hysteresis( COMP_CTX ctx, mujoe_comp_in_hysteresis_t hyst )
{
	uint32_t InputHysteresis;

	switch( hyst )
	{
		case RSDRV_COMP_HYST_10MV:
			InputHysteresis = LL_COMP_HYSTERESIS_10MV;
			break;
		case RSDRV_COMP_HYST_20MV:
			InputHysteresis = LL_COMP_HYSTERESIS_20MV;
			break;
		case RSDRV_COMP_HYST_30MV:
			InputHysteresis = LL_COMP_HYSTERESIS_30MV;
			break;
		case RSDRV_COMP_HYST_40MV:
			InputHysteresis = LL_COMP_HYSTERESIS_40MV;
			break;
		case RSDRV_COMP_HYST_50MV:
			InputHysteresis = LL_COMP_HYSTERESIS_50MV;
			break;
		case RSDRV_COMP_HYST_60MV:
			InputHysteresis = LL_COMP_HYSTERESIS_60MV;
			break;
		case RSDRV_COMP_HYST_70MV:
			InputHysteresis = LL_COMP_HYSTERESIS_70MV;
			break;
		case RSDRV_COMP_HYST_NONE:
		default:
			InputHysteresis = LL_COMP_HYSTERESIS_NONE;
			break;
	}

	LL_COMP_SetInputHysteresis(ctx->instance, InputHysteresis);
	return LL_COMP_GetInputHysteresis(ctx->instance) == InputHysteresis ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_comp_set_input_hysteresis

uint32_t mujoe_comp_read_output_lvl(COMP_CTX ctx)
{
	return LL_COMP_ReadOutputLevel(ctx->instance);
}//mujoe_comp_read_output_lvl

mujoe_err_t mujoe_comp_deactivate(COMP_CTX ctx)
{
	if( LL_COMP_IsEnabled(ctx->instance) == 1 )
	{
		LL_COMP_Disable(ctx->instance);
	}

	return LL_COMP_IsEnabled(ctx->instance) == 0 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_comp_deactivate
