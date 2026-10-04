/*
 * mujoe_hrtim.c
 *
 *  Created on: Sep 21, 2025
 *      Author: Joe Corteo
 */



////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_hrtim.h"
#include "stm32g4xx_ll_exti.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define HRTIM_CLK_SRC_FREQ_HZ			170e6


#define HRTIM_LL_TIMA_THRU_F	LL_HRTIM_TIMER_A |\
								LL_HRTIM_TIMER_B |\
								LL_HRTIM_TIMER_C |\
								LL_HRTIM_TIMER_D |\
								LL_HRTIM_TIMER_E |\
								LL_HRTIM_TIMER_F

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_hrtim_init_context( HRTIM_CTX ctx, HRTIM_HandleTypeDef *hal_hdl, hrtim_ctx_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(hrtim_ctx_t));
	ctx->cfg = *pCfg;
	ctx->hal_hdl = hal_hdl;
	return MUJOE_ERR_NONE;
}// mujoe_gptim_init_context

mujoe_err_t mujoe_hrtim_compareDmaRequestControl( HRTIM_CTX ctx,
												uint32_t timer,
												uint8_t compareCh,
												bool enable )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	switch( compareCh )
	{
		case 1:
			if( enable )
			{
				LL_HRTIM_EnableDMAReq_CMP1(HRTIM, timer);
			}
			else
			{
				LL_HRTIM_DisableDMAReq_CMP1(HRTIM, timer);
			}
			err = ( LL_HRTIM_IsEnabledDMAReq_CMP1(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 2:
			if( enable )
			{
				LL_HRTIM_EnableDMAReq_CMP2(HRTIM, timer);
			}
			else
			{
				LL_HRTIM_DisableDMAReq_CMP2(HRTIM, timer);
			}
			err = ( LL_HRTIM_IsEnabledDMAReq_CMP2(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 3:
			if( enable )
			{
				LL_HRTIM_EnableDMAReq_CMP3(HRTIM, timer);
			}
			else
			{
				LL_HRTIM_DisableDMAReq_CMP3(HRTIM, timer);
			}
			err = ( LL_HRTIM_IsEnabledDMAReq_CMP3(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 4:
			if( enable )
			{
				LL_HRTIM_EnableDMAReq_CMP4(HRTIM, timer);
			}
			else
			{
				LL_HRTIM_DisableDMAReq_CMP4(HRTIM, timer);
			}
			err = ( LL_HRTIM_IsEnabledDMAReq_CMP4(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	return err;

}


mujoe_err_t mujoe_hrtim_captureDmaRequestControl( HRTIM_CTX ctx,
												uint32_t timer,
												uint8_t capture,
												bool enable )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	if( capture == 1 )
	{
		if( enable )
		{
			LL_HRTIM_EnableDMAReq_CPT1(HRTIM, timer);
		}
		else
		{
			LL_HRTIM_DisableDMAReq_CPT1(HRTIM, timer);
		}

		err = ( LL_HRTIM_IsEnabledDMAReq_CPT1(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}
	else if( capture == 2 )
	{
		if( enable )
		{
			LL_HRTIM_EnableDMAReq_CPT2(HRTIM, timer);
		}
		else
		{
			LL_HRTIM_DisableDMAReq_CPT2(HRTIM, timer);
		}

		err = ( LL_HRTIM_IsEnabledDMAReq_CPT2(HRTIM, timer ) == enable ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}
	else
		err = MUJOE_ERR_INV_PARAM;

	return err;
}// mujoe_hrtim_captureDmaRequestControl

void mujoe_hrtim_configBurstDMA( HRTIM_CTX ctx, uint32_t timer, uint32_t registers )
{
	LL_HRTIM_TIM_ConfigBurstDMA(ctx->hal_hdl->Instance, timer, registers);
}// mujoe_hrtim_configBurstDMA

mujoe_err_t mujoe_hrtim_setUpdateGating( HRTIM_CTX ctx, uint32_t timer, uint32_t updateGating )
{
	LL_HRTIM_TIM_SetUpdateGating(ctx->hal_hdl->Instance, timer, updateGating);
	return LL_HRTIM_TIM_GetUpdateGating(ctx->hal_hdl->Instance, timer) == updateGating ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_setUpdateGating

mujoe_err_t mujoe_hrtim_set_tim_prescaler( HRTIM_CTX ctx, uint32_t timer, uint32_t prescaler )
{
	LL_HRTIM_TIM_SetPrescaler(ctx->hal_hdl->Instance, timer, prescaler);
	return LL_HRTIM_TIM_GetPrescaler(ctx->hal_hdl->Instance, timer) == prescaler ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_set_tim_prescaler

uint32_t mujoe_gptim_get_APB2_frequency( void )
{
	LL_RCC_ClocksTypeDef rccClocks;
	LL_RCC_GetSystemClocksFreq(&rccClocks);
	uint32_t apb2_prescaler = LL_RCC_GetAPB2Prescaler();
	uint32_t APB2 = rccClocks.HCLK_Frequency;
	switch( apb2_prescaler )
	{

		case LL_RCC_APB2_DIV_2:
			APB2 >>= 1;
			break;
		case LL_RCC_APB2_DIV_4:
			APB2 >>= 2;
			break;
		case LL_RCC_APB2_DIV_8:
			APB2 >>= 3;
			break;
		case LL_RCC_APB2_DIV_16:
			APB2 >>= 4;
			break;
		case LL_RCC_APB2_DIV_1:
		default:
			break;
	}

	return APB2;
}// mujoe_gptim_get_APB2_frequency


uint32_t mujoe_hrtim_get_f_hrtim( void )
{
	return mujoe_gptim_get_APB2_frequency();
}// mujoe_hrtim_get_f_hrtim



uint32_t mujoe_hrtim_get_tim_freq( HRTIM_CTX ctx, uint32_t timer )
{
	uint32_t pscRatio = LL_HRTIM_TIM_GetPrescaler(ctx->hal_hdl->Instance, timer);
	uint32_t f_hrtim_hz = mujoe_hrtim_get_f_hrtim(); //HRTIM_CLK_SRC_FREQ_HZ;
	switch( pscRatio )
	{
		case LL_HRTIM_PRESCALERRATIO_MUL16:
			//f_hrtim_hz *= 16.0f;
			f_hrtim_hz <<= 4;
			break;
		case LL_HRTIM_PRESCALERRATIO_MUL8:
			//f_hrtim_hz *= 8.0f;
			f_hrtim_hz <<= 3;
			break;
		case LL_HRTIM_PRESCALERRATIO_MUL4:
			//f_hrtim_hz *= 4.0f;
			f_hrtim_hz <<= 2;
			break;
		case LL_HRTIM_PRESCALERRATIO_MUL2:
			//f_hrtim_hz *= 2.0f;
			f_hrtim_hz <<= 1;
			break;
		case LL_HRTIM_PRESCALERRATIO_DIV2:
			//f_hrtim_hz /= 2.0f;
			f_hrtim_hz >>= 1;
			break;
		case LL_HRTIM_PRESCALERRATIO_DIV4:
			//f_hrtim_hz /= 4.0f;
			f_hrtim_hz >>= 2;
			break;
		case LL_HRTIM_PRESCALERRATIO_DIV1:
		default:
			break;

	}

	return f_hrtim_hz;

}// mujoe_hrtim_get_tim_freq

mujoe_err_t mujoe_hrtim_enable_timers(HRTIM_CTX ctx, uint32_t timers)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_TIM_CounterEnable(HRTIM, timers);
	// TODO: Fix this, this cannot be used when input arg timers is a combination of timers e.g. LL_HRTIM_TIMER_A | LL_HRTIM_TIMER_B
	return LL_HRTIM_TIM_IsCounterEnabled(HRTIM, timers) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	//HAL_StatusTypeDef stat = HAL_HRTIM_WaveformCountStart(ctx->hal_hdl, timers);
	//return stat == HAL_OK ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_enable_timers

mujoe_err_t mujoe_hrtim_disable_timers(HRTIM_CTX ctx, uint32_t timers)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_TIM_CounterDisable(HRTIM, timers);
	// TODO: Fix this, this cannot be used when input arg timers is a combination of timers e.g. LL_HRTIM_TIMER_A | LL_HRTIM_TIMER_B
	return LL_HRTIM_TIM_IsCounterEnabled(HRTIM, timers) == 0 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_hrtim_disable_timers

mujoe_err_t mujoe_hrtim_setOutputSetSrc(HRTIM_CTX ctx, uint32_t output, uint32_t setSource)
{
	LL_HRTIM_OUT_SetOutputSetSrc(ctx->hal_hdl->Instance, output, setSource);
	return LL_HRTIM_OUT_GetOutputSetSrc(ctx->hal_hdl->Instance, output ) == setSource ?
			MUJOE_ERR_NONE: MUJOE_ERR_FAILURE;
}// mujoe_hrtim_setOutputSetSrc



// Enables/disables TIMB and TIMC outputs (TB1,TB2,TC1,TC2)
mujoe_err_t mujoe_hrtim_controlGateDriverOutputs(HRTIM_CTX ctx, bool enable)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	if( enable )
	{
		err |= mujoe_hrtim_enable_output(ctx,HRTIM_OUTPUT_TB1);
		err |= mujoe_hrtim_enable_output(ctx,HRTIM_OUTPUT_TB2);
		err |= mujoe_hrtim_enable_output(ctx,HRTIM_OUTPUT_TC1);
		err |= mujoe_hrtim_enable_output(ctx,HRTIM_OUTPUT_TC2);
	}
	else
	{
		err |= mujoe_hrtim_disable_output(ctx,HRTIM_OUTPUT_TB1);
		err |= mujoe_hrtim_disable_output(ctx,HRTIM_OUTPUT_TB2);
		err |= mujoe_hrtim_disable_output(ctx,HRTIM_OUTPUT_TC1);
		err |= mujoe_hrtim_disable_output(ctx,HRTIM_OUTPUT_TC2);

	}

	return err;

}// mujoe_hrtim_controlGateDriverOutputs


mujoe_err_t mujoe_hrtim_enable_output(HRTIM_CTX ctx, uint32_t output)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_EnableOutput(HRTIM, output);
	return LL_HRTIM_IsEnabledOutput(HRTIM, output) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_enable_output


// TODO: Make obsolete
mujoe_err_t mujoe_hrtim_enable_outputs(HRTIM_CTX ctx, uint32_t outputs)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_EnableOutput(HRTIM, outputs);
	// TODO: This is not valid, cannot be a combination of outputs, must be replaced
	return LL_HRTIM_IsEnabledOutput(HRTIM, outputs) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_enable_outputs

// TODO: Make obsolete
mujoe_err_t mujoe_hrtim_disable_outputs(HRTIM_CTX ctx, uint32_t outputs)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_DisableOutput(HRTIM, outputs);
	// TODO: This is not valid, cannot be a combination of outputs, must be replaced
	return LL_HRTIM_IsDisabledOutput(HRTIM, outputs) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_disable_outputs

mujoe_err_t mujoe_hrtim_disable_output(HRTIM_CTX ctx, uint32_t output)
{
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	LL_HRTIM_DisableOutput(HRTIM, output);
	return LL_HRTIM_IsDisabledOutput(HRTIM, output) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_disable_output

mujoe_err_t mujoe_hrtim_enable_all_timers(HRTIM_CTX ctx )
{
	return mujoe_hrtim_enable_timers(ctx, HRTIM_LL_TIMA_THRU_F);
}// mujoe_hrtim_enable_all_timers

mujoe_err_t mujoe_hrtim_disable_all_timers(HRTIM_CTX ctx )
{
	return mujoe_hrtim_disable_timers(ctx, HRTIM_LL_TIMA_THRU_F);
}// mujoe_hrtim_disable_all_timers

mujoe_err_t mujoe_hrtim_TIMx_controlCompareXInterrupt( HRTIM_CTX ctx,
		uint32_t timer,
		uint8_t compare_ch,
		bool enable )
{

	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;

	switch( compare_ch )
	{
		case 1:
			if( LL_HRTIM_IsEnabledIT_CMP1(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CMP1(HRTIM, timer);
					LL_HRTIM_EnableIT_CMP1(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CMP1(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CMP1(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		case 2:
			if( LL_HRTIM_IsEnabledIT_CMP2(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CMP2(HRTIM, timer);
					LL_HRTIM_EnableIT_CMP2(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CMP2(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CMP2(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		case 3:
			if( LL_HRTIM_IsEnabledIT_CMP3(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CMP3(HRTIM, timer);
					LL_HRTIM_EnableIT_CMP3(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CMP3(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CMP3(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		case 4:
			if( LL_HRTIM_IsEnabledIT_CMP4(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CMP4(HRTIM, timer);
					LL_HRTIM_EnableIT_CMP4(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CMP4(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CMP4(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}


	return err;
}// mujoe_hrtim_TIMx_controlCompareXInterrupt


mujoe_err_t mujoe_hrtim_TIMx_controlCaptureXInterrupt( HRTIM_CTX ctx,
		uint32_t timer,
		uint8_t cap_ch,
		bool enable )
{

	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;

	switch( cap_ch )
	{
		case 1:
			if( LL_HRTIM_IsEnabledIT_CPT1(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CPT1(HRTIM, timer);
					LL_HRTIM_EnableIT_CPT1(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CPT1(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CPT1(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		case 2:
			if( LL_HRTIM_IsEnabledIT_CPT2(HRTIM, timer) != enable )
			{
				if( enable )
				{
					// Clear any pending flag to prevent immediate ISR invocation upon enabling
					LL_HRTIM_ClearFlag_CPT2(HRTIM, timer);
					LL_HRTIM_EnableIT_CPT2(HRTIM, timer);
				}
				else
				{
					LL_HRTIM_DisableIT_CPT2(HRTIM, timer);
				}

				err = LL_HRTIM_IsEnabledIT_CPT2(HRTIM, timer) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			}
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}


	return err;
}// mujoe_hrtim_TIMx_controlCompareXInterrupt

mujoe_err_t mujoe_hrtim_TIMx_cntrl_interrupt( HRTIM_CTX ctx, uint32_t timer, bool enable )
{
	mujoe_err_t err =  MUJOE_ERR_NONE;
	IRQn_Type irq = HRTIM1_TIMA_IRQn;

	switch( timer )
	{
		case LL_HRTIM_TIMER_A:
			irq = HRTIM1_TIMA_IRQn;
			break;
		case LL_HRTIM_TIMER_B:
			irq = HRTIM1_TIMB_IRQn;
			break;
		case LL_HRTIM_TIMER_C:
			irq = HRTIM1_TIMC_IRQn;
			break;
		case LL_HRTIM_TIMER_D:
			irq = HRTIM1_TIMD_IRQn;
			break;
		case LL_HRTIM_TIMER_E:
			irq = HRTIM1_TIME_IRQn;
			break;
		case LL_HRTIM_TIMER_F:
			irq = HRTIM1_TIMF_IRQn;
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	if( err == MUJOE_ERR_NONE )
	{
		if( enable )
		{
			NVIC_ClearPendingIRQ(irq);
			// Configure and enable the HRTIM interrupt in the NVIC.
			// Use the specific IRQn for HRTIM1. This may vary by MCU.
			// In many STM32G4s, HRTIM1 has its own dedicated IRQn.
			// Configure interrupt priority
#if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
			// Must be of lower priority than configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (numerically higher)
			NVIC_SetPriority(irq, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1 );
#else
			NVIC_SetPriority(irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
#endif // #if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)

			NVIC_EnableIRQ(irq);
		}
		else
			NVIC_DisableIRQ(irq);
	}
	return err;

}

mujoe_err_t mujoe_hrtim_TIMx_set_period( HRTIM_CTX ctx, uint32_t timer, uint32_t period )
{
	LL_HRTIM_TIM_SetPeriod( ctx->hal_hdl->Instance, timer,  period);
	return LL_HRTIM_TIM_GetPeriod(ctx->hal_hdl->Instance, timer) == period ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_hrtim_TIMx_set_period


// Set Compare X value for Timer Y
mujoe_err_t mujoe_hrtim_TIMx_setCompareX( HRTIM_CTX ctx, uint32_t timer, uint8_t compare_ch, uint32_t compare_val)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hal_hdl->Instance;
	switch( compare_ch )
	{
		case 1:
			LL_HRTIM_TIM_SetCompare1(HRTIM, timer, compare_val);
			err = LL_HRTIM_TIM_GetCompare1(HRTIM, timer) == compare_val ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 2:
			LL_HRTIM_TIM_SetCompare2(HRTIM, timer, compare_val);
			err = LL_HRTIM_TIM_GetCompare2(HRTIM, timer) == compare_val ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 3:
			LL_HRTIM_TIM_SetCompare3(HRTIM, timer, compare_val);
			err = LL_HRTIM_TIM_GetCompare3(HRTIM, timer) == compare_val ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		case 4:
			LL_HRTIM_TIM_SetCompare4(HRTIM, timer, compare_val);
			err = LL_HRTIM_TIM_GetCompare4(HRTIM, timer) == compare_val ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	return err;

}//mujoe_hrtim_TIMx_setCompareX

uint32_t mujoe_hrtim_getOutputLevel( HRTIM_CTX ctx, uint32_t output )
{
	return LL_HRTIM_OUT_GetLevel( ctx->hal_hdl->Instance, output );
}// mujoe_hrtim_getOutputLevel

void mujoe_hrtim_TIMx_counter_reset( HRTIM_CTX ctx, uint32_t timers )
{
	LL_HRTIM_CounterReset(ctx->hal_hdl->Instance, timers);
}// mujoe_hrtim_TIMx_counter_reset


void mujoe_hrtim_TIMx_forceUpdate( HRTIM_CTX ctx, uint32_t timers )
{
	 LL_HRTIM_ForceUpdate(ctx->hal_hdl->Instance, timers);

}// mujoe_hrtim_TIMx_forceUpdate

// Updates a single timer
void mujoe_hrtim_TIMx_forceUpdate_single( HRTIM_CTX ctx, uint32_t timer )
{
	 LL_HRTIM_ForceUpdate(ctx->hal_hdl->Instance, timer);

	 if( LL_HRTIM_IsActiveFlag_UPDATE(ctx->hal_hdl->Instance, timer) == 1 )
	 {
		 LL_HRTIM_ClearFlag_UPDATE(ctx->hal_hdl->Instance, timer);
	 }

}// mujoe_hrtim_TIMx_forceUpdate


void mujoe_hrtim_TIMx_forceUpdateAll( HRTIM_CTX ctx )
{
	 mujoe_hrtim_TIMx_forceUpdate( ctx, HRTIM_LL_TIMA_THRU_F);

}// mujoe_hrtim_TIMx_forceUpdateAll

void mujoe_hrtim_TIMx_counter_reset_all( HRTIM_CTX ctx )
{
	mujoe_hrtim_TIMx_counter_reset( ctx, HRTIM_LL_TIMA_THRU_F );
}// mujoe_hrtim_TIMx_counter_reset_all

mujoe_err_t mujoe_hrtim_dma_burst_config( HRTIM_CTX ctx, hrtim_dma_burst_cfg_t *pCfg)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	LL_DMA_DisableChannel(pCfg->DMA, pCfg->dmaChannel);

	// Ensure that DMA channel is disabled...
	if( LL_DMA_IsEnabledChannel(pCfg->DMA, pCfg->dmaChannel) == 0 )
	{


		// Match reference HRTIM burst DMA: memory buffer -> BDMADR, peripheral address fixed
		uint32_t transferDir = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
		uint32_t periphSize = LL_DMA_PDATAALIGN_WORD;
		uint32_t memSize = LL_DMA_MDATAALIGN_WORD;
		uint32_t periphInc = LL_DMA_PERIPH_NOINCREMENT;
		uint32_t memInc = LL_DMA_MEMORY_INCREMENT;


		LL_DMA_SetDataTransferDirection(pCfg->DMA, pCfg->dmaChannel, transferDir);
		err |= LL_DMA_GetDataTransferDirection(pCfg->DMA, pCfg->dmaChannel) == transferDir ?
				MUJOE_ERR_NONE :MUJOE_ERR_FAILURE;

		// Configure source and destination addresses
		LL_DMA_ConfigAddresses(pCfg->DMA,
				pCfg->dmaChannel,
				pCfg->SrcAddress,
				(uint32_t )&(HRTIM1_COMMON->BDMADR),
				transferDir);

		// Set and confirm DMA request ID for DMA MUX
		LL_DMA_SetPeriphRequest(pCfg->DMA, pCfg->dmaChannel, pCfg->dmaRequestId);
		err |= LL_DMA_GetPeriphRequest(pCfg->DMA, pCfg->dmaChannel) == pCfg->dmaRequestId ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		// Set and confirm data length
		LL_DMA_SetDataLength(pCfg->DMA, pCfg->dmaChannel, pCfg->numRegisters);
		err |= LL_DMA_GetDataLength(pCfg->DMA, pCfg->dmaChannel) == pCfg->numRegisters ?
				MUJOE_ERR_NONE :MUJOE_ERR_FAILURE;

		// Set and confirm priority level
		LL_DMA_SetChannelPriorityLevel(pCfg->DMA, pCfg->dmaChannel, pCfg->priority);
		err |= LL_DMA_GetChannelPriorityLevel(pCfg->DMA, pCfg->dmaChannel)== pCfg->priority ?
				MUJOE_ERR_NONE :MUJOE_ERR_FAILURE;

		// Set and confirm mode
		LL_DMA_SetMode(pCfg->DMA, pCfg->dmaChannel, pCfg->circularModeEn ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL);
		err |= (LL_DMA_GetMode(pCfg->DMA, pCfg->dmaChannel) ==
				(pCfg->circularModeEn ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL)) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		// Set and confirm increment modes
		LL_DMA_SetPeriphIncMode(pCfg->DMA, pCfg->dmaChannel, periphInc );
		err |= LL_DMA_GetPeriphIncMode(pCfg->DMA, pCfg->dmaChannel) == periphInc ?
				MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_SetMemoryIncMode(pCfg->DMA, pCfg->dmaChannel, memInc);
		err |= LL_DMA_GetMemoryIncMode(pCfg->DMA, pCfg->dmaChannel) == memInc ?
				MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		// Set and confirm data sizes for memory and peripheral
		LL_DMA_SetPeriphSize(pCfg->DMA, pCfg->dmaChannel, periphSize);
		err |= LL_DMA_GetPeriphSize(pCfg->DMA,pCfg->dmaChannel) == periphSize ?
				MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_SetMemorySize(pCfg->DMA, pCfg->dmaChannel, memSize);
		err |= LL_DMA_GetMemorySize(pCfg->DMA,pCfg->dmaChannel) == memSize ?
				MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;


		if( pCfg->enableInterrupts )
		{

			// Enable transfer complete interrupt
			LL_DMA_EnableIT_TC(pCfg->DMA, pCfg->dmaChannel);
			// Enable transfer error interrupt
			LL_DMA_EnableIT_TE(pCfg->DMA, pCfg->dmaChannel);

		}
		else
		{
			// Disable transfer complete interrupt
			LL_DMA_DisableIT_TC(pCfg->DMA, pCfg->dmaChannel);
			// Disable transfer error interrupt
			LL_DMA_DisableIT_TE(pCfg->DMA, pCfg->dmaChannel);
		}

		err |= LL_DMA_IsEnabledIT_TC(pCfg->DMA, pCfg->dmaChannel) == pCfg->enableInterrupts ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
		err |= LL_DMA_IsEnabledIT_TE(pCfg->DMA, pCfg->dmaChannel) == pCfg->enableInterrupts ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_EnableChannel(pCfg->DMA, pCfg->dmaChannel);
		if( err == MUJOE_ERR_NONE )
		{
			err = LL_DMA_IsEnabledChannel(pCfg->DMA, pCfg->dmaChannel) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
		}
		else
		{
			err = MUJOE_ERR_FAILURE;
		}

	}
	else
		err = MUJOE_ERR_FAILURE;


	return err;
} // mujoe_hrtim_dma_burst_config

