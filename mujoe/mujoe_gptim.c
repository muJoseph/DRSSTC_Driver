/*
 * mujoe_gptim.c
 *
 *  Created on: Sep 17, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_gptim.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

static const TIM_TypeDef* gptmr_instances[NUM_GP_TIMERS] =
{
		TIM2,
		TIM3,
		TIM4,
		TIM5
};

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_gptim_init_context( GPTIM_CTX ctx, gptim_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(gptim_ctx_t));
	ctx->cfg = *pCfg;
	if( ctx->cfg.id < NUM_GP_TIMERS)
	{
		ctx->instance = (TIM_TypeDef*)gptmr_instances[ctx->cfg.id];
		return MUJOE_ERR_NONE;
	}
	else
		return MUJOE_ERR_INV_PARAM;

}// mujoe_gptim_init_context

mujoe_err_t mujoe_gptim_init_hardware( GPTIM_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	//ctx->instance
	return err;
}// mujoe_gptim_init_hardware

uint32_t mujoe_gptim_get_APB1_frequency( void )
{
	LL_RCC_ClocksTypeDef rccClocks;
	LL_RCC_GetSystemClocksFreq(&rccClocks);
	uint32_t apb1_prescaler = LL_RCC_GetAPB1Prescaler();
	uint32_t APB1 = rccClocks.HCLK_Frequency;
	switch( apb1_prescaler )
	{

		case LL_RCC_APB1_DIV_2:
			APB1 >>= 1;
			break;
		case LL_RCC_APB1_DIV_4:
			APB1 >>= 2;
			break;
		case LL_RCC_APB1_DIV_8:
			APB1 >>= 3;
			break;
		case LL_RCC_APB1_DIV_16:
			APB1 >>= 4;
			break;
		case LL_RCC_APB1_DIV_1:
		default:
			break;
	}

	return APB1;
}// mujoe_gptim_get_APB1_frequency



mujoe_err_t mujoe_gptim_restart( GPTIM_CTX ctx )
{
	// Stop counter
	LL_TIM_DisableCounter(ctx->instance);

	// Disable interrupts if neccessary
	if( ctx->cfg.interruptsEnabled)
		LL_TIM_DisableIT_UPDATE(ctx->instance);

	// Reset count to zero
	LL_TIM_SetCounter(ctx->instance, 0);
	// Generate update event to force reloads
	LL_TIM_GenerateEvent_UPDATE(ctx->instance);
	// Clear update flag to prevent immidiate ISR trigger
	LL_TIM_ClearFlag_UPDATE(ctx->instance);

	// Enable interrupts if neccessary
	if( ctx->cfg.interruptsEnabled)
		LL_TIM_EnableIT_UPDATE(ctx->instance);

	// Re-enable counter
	LL_TIM_EnableCounter(ctx->instance);

	return LL_TIM_IsEnabledCounter(ctx->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_gptim_restart

void mujoe_gptim_generate_update_event( GPTIM_CTX ctx )
{
	LL_TIM_GenerateEvent_UPDATE(ctx->instance);
	if( LL_TIM_IsActiveFlag_UPDATE(ctx->instance) == 1 )
	{
		LL_TIM_ClearFlag_UPDATE(ctx->instance);
	}

}// mujoe_gptim_generate_update_event

mujoe_err_t mujoe_gptim_set_ARR( GPTIM_CTX ctx, uint32_t arr )
{
	// Set auto-reload (ARR)
	LL_TIM_SetAutoReload(ctx->instance, arr);
	return LL_TIM_GetAutoReload(ctx->instance) == arr ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_gptim_set_ARR

mujoe_err_t mujoe_gptim_set_prescaler( GPTIM_CTX ctx, uint32_t psc )
{
	// Set prescaler (PSC)
	LL_TIM_SetPrescaler(ctx->instance, psc);
	return LL_TIM_GetPrescaler(ctx->instance) == psc ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_gptim_set_prescaler

mujoe_err_t mujoe_gptim_ctrl_arrPreload( GPTIM_CTX ctx, bool enable )
{
	if( enable )
	{
		LL_TIM_EnableARRPreload(ctx->instance);
	}
	else
	{
		LL_TIM_DisableARRPreload(ctx->instance);
	}

	return LL_TIM_IsEnabledARRPreload(ctx->instance) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_gptim_ctrl_arrPreload




mujoe_err_t mujoe_gptim_set_compareX_value( GPTIM_CTX ctx,  uint8_t cmpCh, uint16_t cmpVal )
{
	mujoe_err_t err = MUJOE_ERR_FAILURE;

	TIM_TypeDef* GPTIM = ctx->instance;

	switch( cmpCh )
	{
		case 1:
			if( IS_TIM_CC1_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH1(GPTIM, cmpVal);
			    err = LL_TIM_OC_GetCompareCH1(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		case 2:
			if( IS_TIM_CC2_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH2(GPTIM, cmpVal);
				err = LL_TIM_OC_GetCompareCH2(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		case 3:
			if( IS_TIM_CC3_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH3(GPTIM, cmpVal);
				err = LL_TIM_OC_GetCompareCH3(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		case 4:
			if( IS_TIM_CC4_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH4(GPTIM, cmpVal);
				err = LL_TIM_OC_GetCompareCH4(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		case 5:
			if( IS_TIM_CC5_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH5(GPTIM, cmpVal);
				err = LL_TIM_OC_GetCompareCH5(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		case 6:
			if( IS_TIM_CC6_INSTANCE(GPTIM) )
			{
				LL_TIM_OC_SetCompareCH6(GPTIM, cmpVal);
				err = LL_TIM_OC_GetCompareCH6(GPTIM) == cmpVal ? MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
			}
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	return err;

}// mujoe_gptim_set_compareX_value

void mujoe_gptim_setCounter( GPTIM_CTX ctx, uint32_t counter )
{
	LL_TIM_SetCounter(ctx->instance, counter);
}// mujoe_gptim_setCounter

uint32_t mujoe_gptim_compute_compare_val( GPTIM_CTX ctx, uint32_t period_nsec )
{
	float apb1_freq = mujoe_gptim_get_APB1_frequency();
	uint32_t psc = LL_TIM_GetPrescaler(ctx->instance);
	uint32_t arr = LL_TIM_GetAutoReload(ctx->instance);

	//float cnt_freq = apb1_freq/((psc + 1)*(arr + 1));
	// Compute the tick period of timer given prescaler and auto reload
	float cnt_period_sec = ((psc + 1)*(arr + 1))/apb1_freq;
	// Compute compare value to achieve desired period: period_nsec
	uint32_t compare_val = (((float)period_nsec)*1e-9)/cnt_period_sec;
	return compare_val;

}// mujoe_gptim_compute_compare_val

mujoe_err_t mujoe_gptim_channel_control( GPTIM_CTX ctx, uint32_t Channels, bool enable )
{
	if( enable )
	{
		LL_TIM_CC_EnableChannel( ctx->instance, Channels );
	}
	else
	{
		LL_TIM_CC_DisableChannel( ctx->instance, Channels );
	}

	return LL_TIM_CC_IsEnabledChannel(ctx->instance, Channels) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_gptim_channel_control

mujoe_err_t mujoe_gptim_controlAllOutputs( GPTIM_CTX ctx, bool enable )
{
	if( enable )
	{
		LL_TIM_EnableAllOutputs(ctx->instance);
	}
	else
	{
		LL_TIM_DisableAllOutputs(ctx->instance);
	}

	return LL_TIM_IsEnabledAllOutputs(ctx->instance) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_gptim_controlAllOutputs


// Sets the tick frequency of the timer
mujoe_err_t mujoe_gptim_set_update_evt_period( GPTIM_CTX ctx, uint32_t period_nsec )
{
    float apb1_freq = mujoe_gptim_get_APB1_frequency();
    uint32_t arr = (apb1_freq/ (1.0f/((float)period_nsec*1e-9))) - 1;


	// If not 32bit, clamp to 16-bit
	if( !IS_TIM_32B_COUNTER_INSTANCE(ctx->instance) )
	{
		arr &= (uint32_t)0x0000FFFF;
	}

	LL_TIM_SetPrescaler(ctx->instance, 0); // Set the prescaler value
	LL_TIM_SetAutoReload(ctx->instance, arr); // Set the auto-reload value
	// Forces timer to update registers in case preload is enabled
	mujoe_gptim_generate_update_event(ctx);
	return MUJOE_ERR_NONE;
}// mujoe_gptim_set_update_evt_period


static mujoe_err_t start_preset0( GPTIM_CTX ctx, uint32_t Channels)
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	/**********************************/
	/* Start output signal generation */
	/**********************************/
	/* Enable TIMx channels*/
	LL_TIM_CC_EnableChannel(ctx->instance, Channels);

	/* Enable TIMx outputs */
	LL_TIM_EnableAllOutputs(ctx->instance);

	/* Enable counter */
	LL_TIM_EnableCounter(ctx->instance);
	err |= LL_TIM_IsEnabledCounter(ctx->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	/****************************/
	/* TIMx DMA requests set-up */
	/****************************/
	/* Enable DMA request on update event */
	LL_TIM_EnableDMAReq_UPDATE(ctx->instance);

	// Enable update request on compare 2 match
	LL_TIM_EnableDMAReq_CC2(ctx->instance);

	// DEBUG
	//uint32_t enabled = LL_TIM_IsEnabledDMAReq_UPDATE(instance);
	//enabled = LL_TIM_IsEnabledUpdateEvent(instance);

	// Generate update event to force ARR load from DMA
	//LL_TIM_GenerateEvent_UPDATE(ctx->instance);

	// Generate update to force load preload values into regs
	mujoe_gptim_generate_update_event( ctx );

	return err;
} // start_preset0

static mujoe_err_t start_preset1( GPTIM_CTX ctx, uint32_t Channels)
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	/**************************/
	/* TIM1 interrupts set-up */
	/**************************/

	/* Enable the update interrupt */
	//LL_TIM_EnableIT_UPDATE(ctx->instance);

	/**********************************/
	/* Start output signal generation */
	/**********************************/
	/* Enable TIMx channels*/
	LL_TIM_CC_EnableChannel(ctx->instance, Channels);

	/* Enable TIMx outputs */
	LL_TIM_EnableAllOutputs(ctx->instance);

	/* Enable counter */
	LL_TIM_EnableCounter(ctx->instance);
	err |= LL_TIM_IsEnabledCounter(ctx->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Generate update to force load preload values into regs
	mujoe_gptim_generate_update_event( ctx );

	return err;
} // start_preset1

mujoe_err_t mujoe_gptim_startTimer( GPTIM_CTX ctx)
{
	mujoe_err_t err = MUJOE_ERR_NONE;


	if( (err = mujoe_gptim_stop(ctx)) == MUJOE_ERR_NONE )
	{
		TIM_TypeDef *TMR = ctx->instance;
		// Clear counter
		LL_TIM_SetCounter(TMR,0);

		// Enable counter
		LL_TIM_EnableCounter(TMR);
		err = LL_TIM_IsEnabledCounter(TMR) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}

	return err;

}// mujoe_gptim_startTimer

mujoe_err_t mujoe_gptim_start( GPTIM_CTX ctx, uint32_t Channels )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Stop timer
	if( (err |= mujoe_gptim_stop( ctx )) == MUJOE_ERR_NONE  )
	{
		// TIMx.CNT = 0
		LL_TIM_SetCounter(ctx->instance, 0);

		if (ctx->cfg.preset_id == GPTIM_CFG_PRESET_PWM_GEN_DMA )
		{
			err |= start_preset0(ctx, Channels);
		}
		else if( ctx->cfg.preset_id == GPTIM_CFG_PRESET_SLAVE_RST_W_TRGO_GEN )
		{
			err |= start_preset1(ctx, Channels);
		}
		else
			err = MUJOE_ERR_NOT_IMPLEMENTED;
	}

	return err;

}// mujoe_gptim_start


mujoe_err_t mujoe_gptim_stop( GPTIM_CTX ctx)
{
	LL_TIM_DisableCounter(ctx->instance);
	return LL_TIM_IsEnabledCounter(ctx->instance) == 0 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_gptim_stop
