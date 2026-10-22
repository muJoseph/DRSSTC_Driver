/*
 * mujoe_rsdrv.c
 *
 *  Created on: Sep 15, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_hrtim_timx_settings.h"
#include "mujoe_timx_settings.h"
#include "mujoe_resdrv.h"
#include "mujoe_hrtim_eev_settings.h"
#include "mujoe_gpio.h"
#include <string.h>

#if RES_DRV_DBG_EN == 1
#include "mujoe_hrtim_debug.h"
#endif // #if RES_DRV_DBG_EN == 1

#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

// DAC that feeds -ive input of OCD comparator and sets OCD current limit
#define OCD_REF_DAC_ID			DAC3_ID
#define OCD_REF_DAC_CH_ID		MUJOE_DAC_CH1

#define ZCD1_REF_DAC_ID			DAC1_ID
#define ZCD1_REF_DAC_CH_ID		MUJOE_DAC_CH1
#define ZCD2_REF_DAC_ID			DAC1_ID
#define ZCD2_REF_DAC_CH_ID		MUJOE_DAC_CH2

#define MSTR_PER_REF_TIM_ID		TIM2_ID
#define BURST_GEN_TIM_ID		TIM5_ID

#define ZCD_COMP_ID				COMP1_ID
#define OCD_COMP_ID				COMP3_ID

#define TIM2_CLK_FREQ_HZ		170e6

#define HRTIM_TIMx_CLK_FREQ_HZ	170e6

#define ZCD_THRESH_VOLTAGE_V	1.65f 	// V
#define MCU_SUPPLY_VOLTAGE		3.3f	// V

////////////////////////////////////////////////////////////////////////
// LOCAL PROTOS
////////////////////////////////////////////////////////////////////////

static mujoe_err_t init_hrtim( RESDRV_CTX ctx );
static void mujoe_resdrv_updateFlags( RESDRV_CTX ctx,
							   resdrv_fault_flag_t clearMask,
							   resdrv_fault_flag_t setMask  );

static bool isStateChangeValid( RESDRV_CTX ctx, resdrv_sys_state_t newState );

static mujoe_err_t set_zcdFeedback_timeout( RESDRV_CTX ctx, float timeout_usec );

static mujoe_err_t config_masterRefTimer( RESDRV_CTX ctx );
static mujoe_err_t config_zcd_feedback_blanking( RESDRV_CTX ctx );
static mujoe_err_t config_pulseSkip_period( RESDRV_CTX ctx );


static mujoe_err_t config_dma(  RESDRV_CTX ctx );
static inline mujoe_err_t config_resPerCapDma(RESDRV_CTX ctx);


////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////

// DMA2 Callbacks
static void dma2TransferComplete_CB(uint8_t dmaCh);
static void dma2TransferError_CB(uint8_t dmaCh);
static const dma_callbacks_t dma2Cbs =
{
		.transferCompleteCB = dma2TransferComplete_CB,
		.transferErrorCB = dma2TransferError_CB,

};


////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

static const uint32_t OENR_ENABLE_BRIDGE_DRV_OUT = HRTIM_OENR_TB1OEN | HRTIM_OENR_TB2OEN | HRTIM_OENR_TC1OEN | HRTIM_OENR_TC2OEN;

// Pulse Skipping

// Refer to Figure 286 (pg.986) of reference manual for order of register write(s)
// during a Burst DMA transaction
#define PULSE_SKIP_OUT_SET_LEN		4


// TODO: Link these defines to be updated from params mujoe_hrtim_timx_settings.c
#define HRTIM_SETB1R_REG_VAL						HRTIM_SET1R_CMP2							// TB1 Output Set Sources
#define HRTIM_SETB2R_REG_VAL						HRTIM_SET2R_EXTVNT2 | HRTIM_SET2R_EXTVNT6	// TB2 Output Set Sources

#define HRTIM_SETC1R_REG_VAL						HRTIM_SET1R_EXTVNT2 | HRTIM_SET1R_EXTVNT6	// TC1 Output Set Sources
#define HRTIM_SETC2R_REG_VAL						HRTIM_SET2R_CMP2							// TC2 Output Set Sources


// Original Output Configurations
static const uint32_t output_set[PULSE_SKIP_OUT_SET_LEN] =
{
	HRTIM_SETB1R_REG_VAL,							// HRTIM_SETB1R: TIM B Output 1 Set Register
	HRTIM_SETB2R_REG_VAL,							// HRTIM_SETB2R: TIM B Output 2 Set Register
	HRTIM_SETC1R_REG_VAL,							// HRTIM_SETC1R: TIM C Output 1 Set Register
	HRTIM_SETC2R_REG_VAL							// HRTIM_SETC2R: TIM C Output 2 Set Register
};

// Refer to Figure 286 (pg.986) of reference manual for order of register write(s)
// during a Burst DMA transaction
#define PULSE_SKIP_OUT_RESET_LEN		8

// Output configurations in the event of OCD
static const uint32_t output_reset[PULSE_SKIP_OUT_RESET_LEN] =
{
		// First DMA Burst Update: Half-Bridge 1 disabled (i.e. no TB1/2 set sources),
		// current allowed to circulate in Half-Bridge 2 ( driven by TC1/2, i.e. set sources un-molested)
		0x00000000,										// HRTIM_SETB1R: TIM B Output 1 Set Register
		0x00000000,										// HRTIM_SETB2R: TIM B Output 2 Set Register
		HRTIM_SETC1R_REG_VAL,							// HRTIM_SETC1R: TIM C Output 1 Set Register
		HRTIM_SETC2R_REG_VAL,							// HRTIM_SETC2R: TIM C Output 2 Set Register
		// Second DMA Burst Update: Half-Bridge 2 disabled (i.e. no TC1/2 set sources),
		// current allowed to circulate in Half-Bridge 1 ( driven by TB1/2, i.e. set sources un-molested)
		HRTIM_SETB1R_REG_VAL,							// HRTIM_SETB1R: TIM B Output 1 Set Register
		HRTIM_SETB2R_REG_VAL,							// HRTIM_SETB2R: TIM B Output 2 Set Register
		0x00000000,										// HRTIM_SETC1R: TIM C Output 1 Set Register
		0x00000000										// HRTIM_SETC2R: TIM C Output 2 Set Register
};

static RESDRV_CTX _ctx = NULL;	// Ptr to res driver context to be used locally

////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////

#define UPDATE_START_PERIOD_TTICK(F_START)					ctx->rt.start_period = (uint32_t)(HRTIM_TIMx_CLK_FREQ_HZ/((uint32_t)(F_START)));
////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////


// NOTE: Following condition must be satisfied: zcdFeedback_timeout_us <= (65535/f_tick + Tstart)*1e6
// Where f_tick is the HRTIM TIMF tick frequency in Hz
//		 T_start is the burst starting period (1/f_start) (in sec) before the feedback loop is closed
//
void mujoe_resdrv_init_configStruct( presetId_t id, resdrv_cfg_t *pCfg )
{
	memset(pCfg,0,sizeof(resdrv_cfg_t));

	switch( id )
	{
		case RESDRV_PRESET_ID_HIGH_PRI_CURR_STRESS_TEST:
			pCfg->CTBurdenResistor_currFb = 3.3f;
			pCfg->CTRatio_currFb 		 = 1024.0f;
			pCfg->OCDLimit_A			 = 300.0f;
			pCfg->startFreq_Hz			 = 264e3;
			pCfg->startFreq_maxNegDev_percent = 25.0f;
			pCfg->startFreq_maxPosDev_percent = 5.0f;
			pCfg->leadTime_ns			 = 0;
			pCfg->zcdFeedback_timeout_us  = 380.0f;
			// NOTE: Callback en/dis has no affect as of now since
			// it is over-written by ENABLE_DMA_CALLBACKS in mujoe_settings.h
			pCfg->enableDMACallbacks = false;
			pCfg->enableBurstReload = true;
			pCfg->numBurstCycles = 1;
			pCfg->enablePulseSkipping = true;
			pCfg->resPeriodCapBuffLen = MUJOE_RES_PER_CAP_BUF_LEN;
			break;
		case RESDRV_PRESET_ID_DEFAULT:
		default:
			pCfg->CTBurdenResistor_currFb = 10.0f;
			pCfg->CTRatio_currFb 		 = 1024.0f;
			pCfg->OCDLimit_A			 = 100.0f;
			pCfg->startFreq_Hz			 = 264e3;
			pCfg->startFreq_maxNegDev_percent = 25.0f;
			pCfg->startFreq_maxPosDev_percent = 5.0f;
			pCfg->leadTime_ns			 = 0;
			pCfg->zcdFeedback_timeout_us  = 380.0f;
			// NOTE: Callback en/dis has no affect as of now since
			// it is over-written by ENABLE_DMA_CALLBACKS in mujoe_settings.h
			pCfg->enableDMACallbacks = false;
			pCfg->enableBurstReload = true;
			pCfg->numBurstCycles = 1;
			pCfg->enablePulseSkipping = true;
			pCfg->resPeriodCapBuffLen = MUJOE_RES_PER_CAP_BUF_LEN;
			break;
	}
}// mujoe_resdrv_init_configStruct

mujoe_err_t mujoe_resdrv_init_context( RESDRV_CTX ctx,
									   HRTIM_HandleTypeDef *hrtim_hal_hdl,
									   resdrv_cfg_t *pCfg )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	memset(ctx,0,sizeof(resdrv_ctx_t));
	ctx->cfg = *pCfg;

	// Store into local context
	_ctx = ctx;

	// Compute maximum resolvable primary current given Current transformer ratio, burden resistor
	// and STM32 supply voltage
	ctx->rt.Ip_maxResolve = (MCU_SUPPLY_VOLTAGE/ctx->cfg.CTBurdenResistor_currFb) * ctx->cfg.CTRatio_currFb ;

	// This value is to be loaded directly into registers,
	// It assumes PSC=0, and ARR = start_period
	// Note that the Update Frequency (Hz) = f_cnt/(PSC+1)(ARR+1) = f_cnt/(1)(ARR+1) ~= f_cnt/ARR,
	// where f_cnt is the count frequency (timer tick freq)
	// Thus if start_period = f_cnt/f_desired, and ARR = f_cnt/f_desired
	// then Update frequency (Hz) = f_cnt/(f_cnt/f_desired) = f_desired
	UPDATE_START_PERIOD_TTICK(ctx->cfg.startFreq_Hz);

	// Init HRTIM instance
	{
		hrtim_ctx_cfg_t cfg;
		mujoe_hrtim_init_context( &ctx->hrtim, hrtim_hal_hdl, &cfg );

	}
	// Init dac instances
	{
		dac_cfg_t cfg =
		{
				.ch1En = true,
				.ch2En = true,
		};
		err |= mujoe_dac_init_context( &ctx->dacs[DAC1_ID], DAC1, &cfg );
		err |= mujoe_dac_init_context( &ctx->dacs[DAC3_ID], DAC3, &cfg );
	}

	// Init comp instances
	{
		comp_cfg_t cfg;
		err |= mujoe_comp_init_context( &ctx->comps[COMP1_ID], COMP1, &cfg );
		err |= mujoe_comp_init_context( &ctx->comps[COMP2_ID], COMP2, &cfg );
		err |= mujoe_comp_init_context( &ctx->comps[COMP3_ID], COMP3, &cfg );
		err |= mujoe_comp_init_context( &ctx->comps[COMP4_ID], COMP4, &cfg );
	}

	// Init timer instances
	{
		gptim_cfg_t gptim_cfg =
		{
				.id = MUJOE_GPTIM5,
				.preset_id = GPTIM_CFG_PRESET_PWM_GEN_DMA
		};
		err |= mujoe_gptim_init_context( &ctx->gptims[TIM5_ID], &gptim_cfg );

		gptim_cfg.id = MUJOE_GPTIM2;
		gptim_cfg.preset_id = GPTIM_CFG_PRESET_SLAVE_RST_W_TRGO_GEN;
		err |= mujoe_gptim_init_context( &ctx->gptims[TIM2_ID], &gptim_cfg );
	}

	// Init dma instances
	{
		 // DMA 1 is configured in commTask

		 // DMA 2
		 dma_ctx_cfg_t cfg = {.dma_id = DMA_ID_2};
		 mujoe_dma_init_context( &ctx->dma2, &cfg );
		 if(  ctx->cfg.enableDMACallbacks )
		 {
			 mujoe_dma_registerCallbacks( &ctx->dma2, (dma_callbacks_t *)&dma2Cbs );
		 }

	}

	// Init adc instance(s)
	{

		// Initialize ADC sample
		ctx->rt.cs_adc_sample = VAR_CONVERTED_DATA_INIT_VALUE;

		adc_cfg_t cfg =
		{
			.instance = ADC2,
			.differential = true,
			.continuous = true,
			.intEn = true,
			.gainCompensationEn = false,
			.dmaCfg =
			{
				.enable = true,
				.instance = ADC2_DMA,
				.pSampleBuffer = &ctx->rt.cs_adc_sample,
				.enDmaInt = false,
			},
		};

		err |= mujoe_adc_initContext(&ctx->adc2, &cfg );
	}


	return err;
}// mujoe_resdrv_init_context

void mujoe_resdrv_registerCallbacks( RESDRV_CTX ctx, resdrv_cbs_t *pCbs)
{
	ctx->pCbs = pCbs;

}// mujoe_resdrv_registerCallbacks


mujoe_err_t mujoe_resdrv_init_hardware( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_IDLE_DISARMED;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	// Allocate period capture buffer
	if( ctx->cfg.resPeriodCapBuffLen )
	{
		ctx->pResPeriodCapBuff = (uint32_t *)MUJOE_MALLOC(sizeof(uint32_t)*ctx->cfg.resPeriodCapBuffLen);
		if( ctx->pResPeriodCapBuff != NULL )
		{
			memset(ctx->pResPeriodCapBuff, 0, sizeof(uint32_t)*ctx->cfg.resPeriodCapBuffLen);
		}
		else
		{
			return MUJOE_ERR_MALLOC_FAILED;
		}
	}


	// Activate dacs
	for( uint8_t i = 0; i < NUM_DACS;i++)
	{
		if( (err |= mujoe_dac_activate(&ctx->dacs[i])) != MUJOE_ERR_NONE )
			return err;
	}

	// Set DAC feeding OCD comparator INN based on OCD limit
	err |= mujoe_resdrv_set_OCD_limit( ctx, ctx->cfg.OCDLimit_A );

	// Write ZCD threshold voltage to respective dacs
	mujoe_dac_write_voltage(&ctx->dacs[ZCD1_REF_DAC_ID],
								   ZCD1_REF_DAC_CH_ID,
								   ZCD_THRESH_VOLTAGE_V );
	mujoe_dac_write_voltage(&ctx->dacs[ZCD2_REF_DAC_ID],
								   ZCD2_REF_DAC_CH_ID,
								   ZCD_THRESH_VOLTAGE_V );

	// Init ADC hardware
	err |= mujoe_adc_initHardware(&ctx->adc2);
	if( err == MUJOE_ERR_NONE )
	{
		err |= mujoe_adc_cntrlInt_InjEOC( &ctx->adc2, true );

		err |= mujoe_adc_startConversion(&ctx->adc2);

		err |= mujoe_adc_startInjectedConversion( &ctx->adc2 );
	}

	// Activate comps
	for( uint8_t i = 0; i < NUM_COMPS;i++)
	{
		err |= mujoe_comp_activate(&ctx->comps[i]);
	}

	// Init timers
	{
		err |= mujoe_gptim_init_hardware( &ctx->gptims[BURST_GEN_TIM_ID] );
	}

	// Init HRTIM
	{
		err |= init_hrtim(ctx);
	}

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}

	return err;

}// mujoe_resdrv_init_hardware


mujoe_err_t mujoe_resdrv_set_OCD_limit( RESDRV_CTX ctx, float ocd_limit_a )
{
	if (ocd_limit_a > ctx->rt.Ip_maxResolve )
	{
		return MUJOE_ERR_INV_PARAM;
	}
	else
	{
		ctx->rt.V_OCDLimit_ref = (ocd_limit_a / ctx->cfg.CTRatio_currFb) * ctx->cfg.CTBurdenResistor_currFb;
		// Update DAC output
		mujoe_dac_write_voltage(&ctx->dacs[OCD_REF_DAC_ID], OCD_REF_DAC_CH_ID, ctx->rt.V_OCDLimit_ref);
		return MUJOE_ERR_NONE;
	}
}// mujoe_resdrv_set_OCD_limit


mujoe_err_t mujoe_resdrv_set_OCD_comp_input_hysterisis( RESDRV_CTX ctx, mujoe_comp_in_hysteresis_t hyst )
{
	COMP_CTX ocdComp = &ctx->comps[OCD_COMP_ID];
	return mujoe_comp_set_input_hysteresis( ocdComp, hyst);

}// mujoe_resdrv_set_OCD_comp_input_hysterisis

mujoe_err_t reset_all_ext_timers( RESDRV_CTX ctx )
{
	return MUJOE_ERR_NOT_IMPLEMENTED;
}// reset_all_ext_timers

mujoe_err_t mujoe_resdrv_arm( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_IDLE_ARMED;

	if( !isStateChangeValid( ctx, newState ) )
	{
		return MUJOE_ERR_FAILURE;
	}

	if( (err = mujoe_hrtim_disable_all_timers( &ctx->hrtim )) == MUJOE_ERR_NONE )
	{
		// Enable internal HRTIM timers
		// NOTE: This DOES NOT cause the timers to start counting since they are single-shot
		// re-triggerable. Timers must be reset to start counting.
		err |= mujoe_hrtim_enable_all_timers( &ctx->hrtim );
	}

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}
	return err;

}// mujoe_resdrv_arm

mujoe_err_t mujoe_resdrv_disarm( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_IDLE_DISARMED;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	// Disable TB1,TB2,TC1 and TC2
	err |= mujoe_hrtim_controlGateDriverOutputs(&ctx->hrtim, false);

	// Disable all timers
	err |= mujoe_hrtim_disable_all_timers(&ctx->hrtim);

	// Stop TIM2 and reset counter
	err |= mujoe_gptim_stop(&ctx->gptims[TIM2_ID]);
	mujoe_gptim_setCounter( &ctx->gptims[TIM2_ID], 0 );

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}
	return err;

}// mujoe_resdrv_disarm

static mujoe_err_t start_tim2(RESDRV_CTX ctx)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	GPTIM_CTX GPTIM = &ctx->gptims[TIM2_ID];
	err |= mujoe_gptim_startTimer(GPTIM);
	return err;
}// start_tim2

static mujoe_err_t start_tim5(RESDRV_CTX ctx)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	GPTIM_CTX GPTIM = &ctx->gptims[TIM5_ID];

	// Enable DMA request on update event
	LL_TIM_EnableDMAReq_UPDATE(GPTIM->instance);
	err |= LL_TIM_IsEnabledDMAReq_UPDATE(GPTIM->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Enable update request on compare 2 match
	LL_TIM_EnableDMAReq_CC2(GPTIM->instance);
	err |= LL_TIM_IsEnabledDMAReq_CC2(GPTIM->instance) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;


	err |= mujoe_gptim_startTimer(GPTIM);
	return err;
}// start_tim5


mujoe_err_t mujoe_resdrv_stop_burst( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_IDLE_ARMED;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	// Disable all comparators
	err |= mujoe_comp_deactivate(&ctx->comps[COMP1_ID]);
	err |= mujoe_comp_deactivate(&ctx->comps[COMP2_ID]);
	err |= mujoe_comp_deactivate(&ctx->comps[COMP3_ID]);

	err |= mujoe_comp_deactivate(&ctx->comps[COMP4_ID]);

	// Set cnt above CC3 value to force the output to low level
	mujoe_gptim_setCounter( &ctx->gptims[TIM5_ID], ctx->rt.prf_ontime_us + 1 );
	// Stop TIM5
	err |= mujoe_gptim_stop(&ctx->gptims[TIM5_ID]);

	// Clear TIM2 counter
	mujoe_gptim_setCounter( &ctx->gptims[TIM2_ID], 0 );

	// Stop TIM2
	err |= mujoe_gptim_stop(&ctx->gptims[TIM2_ID]);

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}

	return err;
}// mujoe_resdrv_stop_burst

mujoe_err_t mujoe_resdrv_set_lead_time(RESDRV_CTX ctx, uint32_t leadTime_ns)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	if( leadTime_ns > LEAD_TIME_MAX_NSEC )
	{
		return MUJOE_ERR_INV_PARAM;
	}
	// TIM 2 - Lead time and Resonant Freq Injection
	GPTIM_CTX GPTIM = &ctx->gptims[MSTR_PER_REF_TIM_ID];

	float tim_period_ns = (1.0f/(float)TIM2_CLK_FREQ_HZ) * 1e9;
	// Compute compare value
	// Desired period/timer period = compare value
	uint32_t tim2_cmp1 = (uint32_t)(((float)leadTime_ns)/tim_period_ns);

	// Check if compare val exceeds 16-bit full-scale
	if( tim2_cmp1 > 65535 )
	{
		return MUJOE_ERR_INV_PARAM;
	}

	// Set TIM2.CCR1 = lead time
	// TIM2 TRGO pulse is generated when TIM2.CNT = TIM2.CCR1
	if( (err = mujoe_gptim_set_compareX_value( GPTIM,  1, (uint16_t)tim2_cmp1 )) == MUJOE_ERR_NONE )
	{
		ctx->cfg.leadTime_ns = leadTime_ns;

		// Check if master timer
		if( IS_TIM_MASTER_INSTANCE(GPTIM->instance) == 1 )
		{
			// Set TRGO trigger: CC1F Flag set (TIM2 CNT= CMP1) if lead time specified
			// ...set trigger to timer update if NO lead time specified
			// NOTE: Do NOT use LL_TIM_TRGO_RESET when lead time is 0, when
			// in this mode TIM2 TRGO will not be generated on reset/roll-over in the classical
			// timer sense, but instead only when the timer is itself is reset (therefore a misleading naming convention)
			LL_TIM_SetTriggerOutput(GPTIM->instance, ctx->cfg.leadTime_ns == 0 ?
					LL_TIM_TRGO_UPDATE : LL_TIM_TRGO_CC1IF);
		}
	}

	return err;
}// mujoe_resdrv_set_lead_time

static void mujoe_resdrv_updateFlags( RESDRV_CTX ctx,
							   resdrv_fault_flag_t clearMask,
							   resdrv_fault_flag_t setMask  )
{
	resdrv_fault_flag_t newFlag = ctx->rt.fault_flag;
	newFlag &= ~clearMask;
	newFlag |= setMask;
	if( newFlag != ctx->rt.fault_flag )
	{
		ctx->rt.fault_flag = newFlag;
		if( ctx->pCbs && ctx->pCbs->faultChangeCB )
		{
			ctx->pCbs->faultChangeCB( ctx->cfg.hostArg, ctx->rt.fault_flag );
		}
	}

}// mujoe_resdrv_updateFlags


resdrv_fault_flag_t mujoe_resdrv_getFaults( RESDRV_CTX ctx )
{
	return ctx->rt.fault_flag;

}// mujoe_resdrv_getFaults

// Gets the instantaneous resonant frequency
uint32_t mujoe_resdrv_getMeasuredResonantFrequency( RESDRV_CTX ctx )
{
	return HRTIM_TIMx_CLK_FREQ_HZ/HRTIM1_TIME->CPT1xR;
}// mujoe_resdrv_getMeasuredResonantFrequency

mujoe_err_t mujoe_resdrv_restoreHbridgeOutputRouting(RESDRV_CTX ctx)
{
    mujoe_err_t err = MUJOE_ERR_NONE;
	// Reconfigure set sources for Timer B and C outputs: TB1,TB2,TC1,TC2
    err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TB1, hrtim_timB_cfg.outCfg[0].setSrcs);
    err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TB2, hrtim_timB_cfg.outCfg[1].setSrcs);
    err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TC1, hrtim_timC_cfg.outCfg[0].setSrcs);
    err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TC2, hrtim_timC_cfg.outCfg[1].setSrcs);
    return err;
}// mujoe_resdrv_restoreHbridgeOutputRouting

mujoe_err_t mujoe_resdrv_ackFaults( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_IDLE_DISARMED;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	err |= mujoe_resdrv_restoreHbridgeOutputRouting(ctx);

	ctx->rt.fault_flag = 0;

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}

	return err;

}// mujoe_resdrv_ackFaults

void mujoe_resdrv_getResPeriodCapBuffer( RESDRV_CTX ctx, uint32_t **ppBuf, int *pBufLen )
{

	*ppBuf = ctx->pResPeriodCapBuff;
	*pBufLen = ctx->cfg.resPeriodCapBuffLen;

}// mujoe_resdrv_getResPeriodCapBuffer

mujoe_err_t mujoe_resdrv_handleFeedbackTimeout( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_FAULT;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	// Disable TIM F
	err |= mujoe_hrtim_disable_timers(&ctx->hrtim, LL_HRTIM_TIMER_F);

	// Disable all set sources for TB1
	err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TB1, LL_HRTIM_OUTPUTSET_NONE);
	// Disable all set sources for TB2
	err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TB2, LL_HRTIM_OUTPUTSET_NONE);;
	// Disable all set sources for TC1
	err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TC1, LL_HRTIM_OUTPUTSET_NONE);
	// Disable all set sources for TC2
	err |= mujoe_hrtim_setOutputSetSrc(&ctx->hrtim, LL_HRTIM_OUTPUT_TC2, LL_HRTIM_OUTPUTSET_NONE);

	// Wait for TIMB and C Outputs (TB1,TB2,TC1,TC2) to go to INACTIVE state
	while( (mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TB1 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
			(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TB2 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
			(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TC1 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
			(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TC2 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) );

	// Stop burst operation
	err |= mujoe_resdrv_stop_burst(ctx);

	err |= mujoe_resdrv_disarm(ctx);

	// Enter fault state
	if( err == MUJOE_ERR_NONE  )
	{
		mujoe_resdrv_updateFlags( ctx, 0, RESDRV_FAULT_FLAG_FB_TIMEOUT );
		ctx->rt.sys_state = newState;
	}

	return err;
}// mujoe_resdrv_handleFeedbackTimeout

mujoe_err_t mujoe_resdrv_handleNumPrfCyclesComplete( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Stop burst operation
	err |= mujoe_resdrv_stop_burst(ctx);

	err |= mujoe_resdrv_disarm(ctx);

	return err;
}// mujoe_resdrv_handleNumPrfCyclesComplete


static inline mujoe_err_t seed_trigHalfMode( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	// Set HRTIM TIMB CMP2 and TIMC CMP2
	// load cmp2 registers with half of the period for blanking
	// this is updated during the burst by the triggered-half mode
	err |= mujoe_hrtim_TIMx_setCompareX( &ctx->hrtim,
			LL_HRTIM_TIMER_B,
			2,
			ctx->rt.start_period>>1 );
	err |= mujoe_hrtim_TIMx_setCompareX( &ctx->hrtim,
			LL_HRTIM_TIMER_C,
			2,
			ctx->rt.start_period>>1);

	return err;

}// seed_trigHalfMode

static inline mujoe_err_t propagate_start_frequency( RESDRV_CTX ctx,
													uint32_t startFreq_Hz,
													uint8_t maxPosDeviation,
													uint8_t maxNegDeviation)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	GPTIM_CTX GPTIM2 = &ctx->gptims[TIM2_ID];
	HRTIM_CTX hrtim = &ctx->hrtim;

	// Update start_period within context so it can be propagated throughout the driver
	UPDATE_START_PERIOD_TTICK(startFreq_Hz);

	// Update config structure,
	ctx->cfg.startFreq_Hz = startFreq_Hz;
	ctx->cfg.startFreq_maxNegDev_percent = maxNegDeviation;
	ctx->cfg.startFreq_maxPosDev_percent = maxPosDeviation;

	// Update ARR of TIM2
	err |= mujoe_gptim_set_ARR( GPTIM2, ctx->rt.start_period );

	// Refresh TIMA compare reg since it depends on start_period
	err |= config_masterRefTimer(ctx);

	// Seeds the CMP2 reg of TIMB and TIMC. Not really necessary, but done for completeness
	// CMP2 will be updated on TIMB/TIMC Capture 1 (triggered by EEV2 (TIM2 TRGO) || EEV6 (ZCD) )
	// via the triggered-half mode mechanism
	err |= seed_trigHalfMode(ctx);

	// Refresh TIMD period since it depends on start_period
	err |= config_pulseSkip_period( ctx );

	// Refresh ZCD feedback blanking within TIME since it depends on start_period
	err |= config_zcd_feedback_blanking( ctx );

	// Refresh TIMF (zcd timeout) compare reg since it depends on startFreq_hz
	err |= set_zcdFeedback_timeout( ctx, ctx->cfg.zcdFeedback_timeout_us );

	// Gen update for TIMA, TIMB,TIMC, TIMD,TIME, TIMF to force update of any
	// active SFRs that have preload enabled
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_A );
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_B );
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_C );
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_D );
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_E );
	mujoe_hrtim_TIMx_forceUpdate_single(hrtim, LL_HRTIM_TIMER_F );

	// Gen TIM2 update to force active ARR to reflect shadow reg in case ARR preload is enabled
	mujoe_gptim_generate_update_event(GPTIM2);

	return err;
}// propagate_start_frequency




mujoe_err_t mujoe_resdrv_start_burst( RESDRV_CTX ctx, mujoe_resdrv_start_burst_arg_t *pArg )
{

	resdrv_sys_state_t newState = RESDRV_SYS_STATE_RUN;
	if( !isStateChangeValid(ctx, newState) )
	{
		return MUJOE_ERR_FAILURE;
	}

	mujoe_err_t err = MUJOE_ERR_NONE;
	GPTIM_CTX GPTIM5 = &ctx->gptims[TIM5_ID];	// Burst generator gp timer
	GPTIM_CTX GPTIM2 = &ctx->gptims[TIM2_ID];	// Lead time feedback

	// Stop master timer (TIM2)
	err |= mujoe_gptim_stop(GPTIM2);
	// Stop burst gen timer (TIM5)
	err |= mujoe_gptim_stop(GPTIM5);

	if( err == MUJOE_ERR_NONE )
	{

		// Propagates the new starting frequency to the rest
		// of the res driver sub modules
		err |= propagate_start_frequency(ctx,
										pArg->startFreq_Hz,
										pArg->startFreq_maxPosDev_percnt,
										pArg->startFreq_maxNegDev_percnt);

		// If finite PRF cycles specified, clear counter in prep for start
		ctx->cfg.numBurstCycles = pArg->numBurstCycles;
		if( ctx->cfg.numBurstCycles )
		{
			ctx->rt.prfCycleCnt = 0;
		}

		// Enable ZCD and OCD comparators
		err |= mujoe_comp_activate(&ctx->comps[COMP1_ID]);
		err |= mujoe_comp_activate(&ctx->comps[COMP2_ID]);
		err |= mujoe_comp_activate(&ctx->comps[COMP3_ID]);

		// Burst comparator (COMP4) stays active; deactivating it broke closed-loop in testing.
		err |= mujoe_comp_activate(&ctx->comps[COMP4_ID]);

		// Force update TIM A, B and C
		mujoe_hrtim_TIMx_forceUpdate( &ctx->hrtim,
				 	 	 	 	 	  LL_HRTIM_TIMER_A | LL_HRTIM_TIMER_B | LL_HRTIM_TIMER_C );

		// Wait for TIMB and C Outputs (TB1,TB2,TC1,TC2) to go to INACTIVE state
		while( (mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TB1 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
				(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TB2 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
				(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TC1 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) ||
				(mujoe_hrtim_getOutputLevel( &ctx->hrtim, LL_HRTIM_OUTPUT_TC2 ) == LL_HRTIM_OUT_LEVEL_ACTIVE ) );

		// Enable TIMB outputs (TB1, TB2) and TIMC outputs (TC1,TC2)
		err |= mujoe_hrtim_controlGateDriverOutputs(&ctx->hrtim, true);

		// Save the PRF period into local context
		ctx->rt.prf_period_us = pArg->period_us;

		uint16_t tim5_cmp2 = ctx->rt.prf_period_us;
		if( ctx->rt.prf_period_us > BURST_DMARQ_LEADTIME_US )
		{
			tim5_cmp2 = ctx->rt.prf_period_us-BURST_DMARQ_LEADTIME_US;
		}

		// Clamp PRF on-time to min if neccessary, save to local context
		if( pArg->ontime_us < PRF_ON_TIME_MIN_USEC )
		{
			ctx->rt.prf_ontime_us = PRF_ON_TIME_MIN_USEC;
		}

		else
		{
			ctx->rt.prf_ontime_us = pArg->ontime_us;
		}

		// Set TIM5_CCR2
		// Used for DMA request generation:
		// When TIM5.CNT == TIM5.CCR2: ctx->rt.start_period (mem) is loaded into TIM2.ARR register
		// The DMA request is generated BURST_DMARQ_LEADTIME_US usec...
		// ...BEFORE each burst completes from burst generator timer (TIM5)
		err |= mujoe_gptim_set_compareX_value( GPTIM5,
											   2,
											   tim5_cmp2 );
		// Set TIM5_CCR3
		// Used for On-time of burst
		err |= mujoe_gptim_set_compareX_value( GPTIM5,
											   3,
											   ctx->rt.prf_ontime_us );

#if RES_DRV_DBG_EN == 1
		// Set TIM5_CCR1
		// Used for On-time of burst
		// NOTE: This replicates CCR3 but on a GPIO that is more accessible on the NUCLEO board
		err |= mujoe_gptim_set_compareX_value( GPTIM5,
											   1,
											   ctx->rt.prf_ontime_us );
#endif// #if RES_DRV_DBG_EN == 1

		// Set TIM5_CCR4
		// TODO: Currently unused, consider removing
		err |= mujoe_gptim_set_compareX_value( GPTIM5,
											   4,
											   tim5_cmp2 );

		// Set TIM5.PSC
		err |= mujoe_gptim_set_prescaler(GPTIM5, 169 );

		// Set TIM5.ARR
		// Used for burst period (and thus burst frequency)
		err |= mujoe_gptim_set_ARR( GPTIM5, ctx->rt.prf_period_us );

		// Enable TIM5 CH1, CH2 CH3
		err |=  mujoe_gptim_channel_control( GPTIM5,
				LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2 | LL_TIM_CHANNEL_CH3,
				true);

		// Force update of TIM5
		mujoe_gptim_generate_update_event(GPTIM5);

#if RES_DRV_DBG_EN != 1
		// Disable all outputs of TIM2
		err |= mujoe_gptim_controlAllOutputs( MST_TMR_GPTIM, false);
#endif // #if RES_DRV_DBG_EN != 1

		// Re-configure all HRTIM associated DMA channels to ensure clean start
		err |= config_dma(ctx);

		// Enable TIM2 CH1
		err |=  mujoe_gptim_channel_control( GPTIM2, LL_TIM_CHANNEL_CH1, true);

		// Force update of TIM2
		mujoe_gptim_generate_update_event(GPTIM2);

		// Reset all HRTIM timers
		mujoe_hrtim_TIMx_counter_reset(&ctx->hrtim, LL_HRTIM_TIMER_A |
													LL_HRTIM_TIMER_B |
													LL_HRTIM_TIMER_C |
													LL_HRTIM_TIMER_D |
													LL_HRTIM_TIMER_E |
													LL_HRTIM_TIMER_F);
		err |= start_tim2(ctx);
		err |= start_tim5(ctx);

	}

	if( err == MUJOE_ERR_NONE )
	{
		ctx->rt.sys_state = newState;
	}


	return err;

}// mujoe_resdrv_start_burst

////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
////////////////////////////////////////////////////////////////////////

static mujoe_err_t set_zcdFeedback_timeout( RESDRV_CTX ctx, float timeout_usec)
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Compute period of excitation freq in usec
	float T_start_usec = 1e6 * (1.0f/ctx->cfg.startFreq_Hz);
	// Subtract 1 period/cycle from requested timeout to allow finishing
	// current cycle and account for irq entry delay
	float timeout_adj_usec = timeout_usec - T_start_usec;
	//float timeout_adj_usec = timeout_usec + T_start_usec;	// TODO: Evaluate if adding period has any affect on performance

	// Get the tick frequency of HRTIM TIM F
	float timf_freq_hz = mujoe_hrtim_get_tim_freq( &ctx->hrtim, LL_HRTIM_TIMER_F );

	// Compute CMP3 value to achieve adjusted timeout given timer tick frequency
	uint32_t timf_cmp3 = (timeout_adj_usec * 1e-6)*timf_freq_hz;

	// Ensure that compare value does not overflow 16 bit register
	if( timf_cmp3 > 65535 )
	{
		return MUJOE_ERR_INV_PARAM;
	}

	// Set TIMF CMP3 register
	// ZCD Timeout interrupt generated when TIMF CNT = TIMF CMP3
	err |= mujoe_hrtim_TIMx_setCompareX(&ctx->hrtim, LL_HRTIM_TIMER_F, 3, (uint16_t)timf_cmp3);

	// Enable TIMF Compare 3 interrupt
	err |= mujoe_hrtim_TIMx_controlCompareXInterrupt( &ctx->hrtim,
														LL_HRTIM_TIMER_F,
														3,
														true );

	// Unmask HRTIM TIMF interrupts
	err |= mujoe_hrtim_TIMx_cntrl_interrupt( &ctx->hrtim, LL_HRTIM_TIMER_F, true );
	return err;

}//set_zcdFeedback_timeout

// Init Master Period Reference Timer (TIM2)
static mujoe_err_t init_tim2( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	// TIM2 instance
	GPTIM_CTX GPTIM = &ctx->gptims[MSTR_PER_REF_TIM_ID];
	TIM_TypeDef *TIM = GPTIM->instance;

	// Ensure timer is stopped in prep for cfg
	if( (err = mujoe_gptim_stop(GPTIM)) != MUJOE_ERR_NONE )
	{
		return err;
	}

	// Enable ARR preload
	// ARR not loaded until update event. All writes to ARR are to shadow register
	err |= mujoe_gptim_ctrl_arrPreload(GPTIM, true );

	// Set prescaler (PSC)
	err |= mujoe_gptim_set_prescaler(GPTIM, 0);

	// Set auto-reload (ARR)
	err |= mujoe_gptim_set_ARR( GPTIM, ctx->rt.start_period );

	// Cfg TIM2 as Downcounter
	uint32_t cntr_mode = LL_TIM_COUNTERMODE_DOWN;
	LL_TIM_SetCounterMode(TIM, cntr_mode);
	err |= LL_TIM_GetCounterMode(TIM) == cntr_mode ?  MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Configure CC Ch1

	// Set CH1 output config: Frozen
	// We are using this as time-base for TRGO generation
	LL_TIM_OC_SetMode(TIM, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FROZEN);
	err |= LL_TIM_OC_GetMode(TIM,LL_TIM_CHANNEL_CH1) == LL_TIM_OCMODE_FROZEN ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Enable TIM2 cap/compare 1 channel
	// TIM2 TRGO genenerated on TIM2 cmp1 match
	LL_TIM_CC_EnableChannel(TIM, LL_TIM_CHANNEL_CH1);
	err |= LL_TIM_CC_IsEnabledChannel(TIM, LL_TIM_CHANNEL_CH1 ) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;


	// Configure Master/slave mode
	LL_TIM_EnableMasterSlaveMode(TIM);
	if( LL_TIM_IsEnabledMasterSlaveMode(TIM) == 1 )
	{
		//  Cfg master portion of timer
		if( IS_TIM_MASTER_INSTANCE(TIM) == 1 )
		{
			// Set TRGO trigger: CC1F Flag set (TIM2 CNT= CMP1) if lead time specified
			// ...set trigger to timer reset if NO lead time specified
			// NOTE: Do NOT use LL_TIM_TRGO_RESET when lead time is 0, when
			// in this mode TIM2 TRGO will not be generated on reset/roll-over in the classical
			// timer sense, but instead only when the timer is itself is reset (therefore a misleading naming convention)
			LL_TIM_SetTriggerOutput(TIM, ctx->cfg.leadTime_ns == 0 ?
									LL_TIM_TRGO_UPDATE : LL_TIM_TRGO_CC1IF);
		}

		// Cfg slave portion of timer
		if( IS_TIM_SLAVE_INSTANCE(TIM) == 1 )
		{
			// Cfg slave mode: Reset
			// Timer is reset on ITR10 - hrtim_out_sync2
			LL_TIM_SetSlaveMode(TIM, LL_TIM_SLAVEMODE_RESET);
			// Cfg slave mode trigger input: ITR10 - hrtim_out_sync2
			LL_TIM_SetTriggerInput(TIM, LL_TIM_TS_ITR10);
		}
	}


	// Generate update event to force preload
	mujoe_gptim_generate_update_event(GPTIM);

	return err;
}// init_tim2

// Init Burst generator timer (TIM5)
static mujoe_err_t init_tim5( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	GPTIM_CTX GPTIM = &ctx->gptims[BURST_GEN_TIM_ID];
	TIM_TypeDef *TIM = GPTIM->instance;


	// Enable TIM5 cap/compare 3 channel
	// TIM5_CCER.CC3E = 1
	// Used for: Burst ON-Time
	LL_TIM_CC_EnableChannel(TIM, LL_TIM_CHANNEL_CH3);

#if RES_DRV_DBG_EN == 1
	// Enable TIM5 cap/compare 1 channel
	// TIM5_CCER.CC1E = 1
	// Used for: Burst ON-Time
	LL_TIM_CC_EnableChannel(TIM, LL_TIM_CHANNEL_CH1);
#endif // #if RES_DRV_DBG_EN == 1

	// Set prescaler (PSC)
	LL_TIM_SetPrescaler(TIM,170-1);
	// Set auto-reload (ARR)
	LL_TIM_SetAutoReload(TIM, 10);

	// TIM5.CCR3 = 0
	LL_TIM_OC_SetCompareCH3(TIM, 0);

#if RES_DRV_DBG_EN == 1
	// TIM5.CCR1 = 0
	LL_TIM_OC_SetCompareCH1(TIM, 0);
#endif // #if RES_DRV_DBG_EN == 1

	// Enable auto-reload (ARR) preload.
	// TIM5_CR1.ARPE = 1
	err |= mujoe_gptim_ctrl_arrPreload( GPTIM, true );

	// Forces timer to update registers in case preload is enabled
	mujoe_gptim_generate_update_event(GPTIM);

	return err;
}// init_tim5

//#define HRTIM_TIMX_MAX_PERIOD		0x0000FFDF

static mujoe_err_t config_eev_event(RESDRV_CTX ctx, uint32_t event, eev_event_config_t *pCfg )
{
	HRTIM_TypeDef *HRTIM = ctx->hrtim.hal_hdl->Instance;
	uint32_t Configuration = 0;
	Configuration |= pCfg->polarityHigh ? LL_HRTIM_EE_POLARITY_HIGH : LL_HRTIM_EE_POLARITY_LOW;
	Configuration |= pCfg->sensitivity;
	// Fast mode only available for EEV1-5
	if( event < LL_HRTIM_EVENT_6)
	{
		Configuration |= pCfg->fastModeEnable ? LL_HRTIM_EE_FASTMODE_ENABLE : LL_HRTIM_EE_FASTMODE_DISABLE;
	}

	Configuration |= pCfg->eventSrc;

	LL_HRTIM_EE_Config(HRTIM, event, Configuration);

	// Digital filter only available for EEV6-10
	if( event >= LL_HRTIM_EVENT_6)
	{
		LL_HRTIM_EE_SetFilter(HRTIM, event, pCfg->filter);
	}


	return MUJOE_ERR_NONE;
}// config_eev_event



static mujoe_err_t config_eev_mapping( RESDRV_CTX ctx  )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hrtim.hal_hdl->Instance;

	// Configure EEV 1 throu EEV10 events
	uint32_t evt = LL_HRTIM_EVENT_1;
	for( uint8_t i = 0; i < HRTIM_NUM_EEV; i++ )
	{
		err |= config_eev_event(ctx, evt << i, (eev_event_config_t *)&eev_event_cfgs[i] );
	}


#if RES_DRV_DBG_EN == 1
	// Read sources for each EEV as a sanity check
    uint8_t eevSources[HRTIM_NUM_EEV] = {0};
    evt = LL_HRTIM_EVENT_1;
	for( uint8_t i = 0; i < HRTIM_NUM_EEV; i++ )
	{
		eevSources[i] = LL_HRTIM_EE_GetSrc(HRTIM, evt << i);
	}
#endif // #if RES_DRV_DBG_EN == 1

	return err;

}// config_eev_mapping


static mujoe_err_t init_hrtim_timx( RESDRV_CTX ctx, uint32_t timer, hrtim_timx_cfg_t *pCfg )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	HRTIM_TypeDef *HRTIM = ctx->hrtim.hal_hdl->Instance;

	// Disable counter
	LL_HRTIM_TIM_CounterDisable(HRTIM, timer);
	if( LL_HRTIM_TIM_IsCounterEnabled(HRTIM,timer) == 1 )
	{
		return MUJOE_ERR_FAILURE;
	}


	// Set and confirm prescaler
	LL_HRTIM_TIM_SetPrescaler(HRTIM, timer, pCfg->pscRatio);
	err |= LL_HRTIM_TIM_GetPrescaler(HRTIM, timer) == pCfg->pscRatio ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Set and confirm timer period
	LL_HRTIM_TIM_SetPeriod(HRTIM, timer, pCfg->timPeriod);
	err |= LL_HRTIM_TIM_GetPeriod(HRTIM, timer) == pCfg->timPeriod ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Set and confirm update trigger
	LL_HRTIM_TIM_SetUpdateTrig(HRTIM, timer, pCfg->updateTrig);
	err |= LL_HRTIM_TIM_GetUpdateTrig(HRTIM, timer) == pCfg->updateTrig ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	// Set timer counting mode: up-counter
	LL_HRTIM_TIM_SetCountingMode(HRTIM, timer, pCfg->countingMode);
	// Set counter mode: re-triggerable single-shot
	LL_HRTIM_TIM_SetCounterMode(HRTIM, timer, pCfg->counterMode);

	// Enable triggered half mode
	LL_HRTIM_TIM_SetTriggeredHalfMode(HRTIM, timer, pCfg->triggerHalfModeEn ? LL_HRTIM_TRIGHALF_ENABLED : LL_HRTIM_TRIGHALF_DISABLED);
	err |= LL_HRTIM_TIM_GetTriggeredHalfMode(HRTIM, timer) ==
			(pCfg->triggerHalfModeEn ? LL_HRTIM_TRIGHALF_ENABLED : LL_HRTIM_TRIGHALF_DISABLED) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	// Set Reset Trigger(s)
	// These sources will reset the timer
	LL_HRTIM_TIM_SetResetTrig(HRTIM, timer, pCfg->resetTrig );

	// Configure EEV Blanking filters if they exist
	if( pCfg->pEEVBlankFltrs != NULL )
	{
		// Configure event filtering
		for( int i = 0; i < pCfg->numBlankFltrs; i++ )
		{
			LL_HRTIM_TIM_SetEventFilter(HRTIM,
					timer,
					pCfg->pEEVBlankFltrs[i].event,
					pCfg->pEEVBlankFltrs[i].filter);
			// Set latch status
			LL_HRTIM_TIM_SetEventLatchStatus(HRTIM,
											 timer,
											 pCfg->pEEVBlankFltrs[i].event,
											 pCfg->pEEVBlankFltrs[i].latchEn ? LL_HRTIM_EELATCH_ENABLED : LL_HRTIM_EELATCH_DISABLED);
		}
	}


	// Set capture triggers on capture units
	LL_HRTIM_TIM_SetCaptureTrig(HRTIM, timer, LL_HRTIM_CAPTUREUNIT_1, pCfg->capCfg[0].capTrig);
	LL_HRTIM_TIM_SetCaptureTrig(HRTIM, timer, LL_HRTIM_CAPTUREUNIT_2, pCfg->capCfg[1].capTrig);


	// Configure compare 1
	LL_HRTIM_TIM_SetComp1Mode(HRTIM, timer, pCfg->cmp1EqualToMode ? LL_HRTIM_GTCMP1_EQUAL : LL_HRTIM_GTCMP1_GREATER);

	// Configure compare 3
	LL_HRTIM_TIM_SetComp3Mode(HRTIM, timer, pCfg->cmp3EqualToMode ? LL_HRTIM_GTCMP3_EQUAL : LL_HRTIM_GTCMP3_GREATER);


	// Only configure compare 2 mode if triggered half mode is DISABLED
	if( LL_HRTIM_TIM_GetTriggeredHalfMode(HRTIM, timer) == LL_HRTIM_TRIGHALF_DISABLED )
	{
		// Config and confirm compare 2
		LL_HRTIM_TIM_SetCompareMode(HRTIM, timer, LL_HRTIM_COMPAREUNIT_2, pCfg->cmp2Mode);
		err |= LL_HRTIM_TIM_GetCompareMode(HRTIM, timer, LL_HRTIM_COMPAREUNIT_2) == pCfg->cmp2Mode ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}

	// Configure compare 4
	LL_HRTIM_TIM_SetCompareMode(HRTIM, timer, LL_HRTIM_COMPAREUNIT_4, pCfg->cmp4Mode);
	err |= LL_HRTIM_TIM_GetCompareMode(HRTIM, timer, LL_HRTIM_COMPAREUNIT_4) == pCfg->cmp4Mode ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;


	// Config Delayed Protection
	if( pCfg->dlyPrt.enabled )
	{
		// Set delayed protection mode
		LL_HRTIM_TIM_SetDLYPRTMode(HRTIM, timer, pCfg->dlyPrt.mode);
		// Enable delayed protection
		LL_HRTIM_TIM_EnableDLYPRT(HRTIM, timer);

		if( LL_HRTIM_TIM_GetDLYPRTMode(HRTIM, timer) !=  pCfg->dlyPrt.mode )
			err |= MUJOE_ERR_FAILURE;
		if( LL_HRTIM_TIM_IsEnabledDLYPRT(HRTIM,timer) == 0 )
			err |= MUJOE_ERR_FAILURE;
	}
	else
	{
		// Disable delayed protection
		LL_HRTIM_TIM_DisableDLYPRT(HRTIM, timer);
		if( LL_HRTIM_TIM_IsEnabledDLYPRT(HRTIM,timer) == 1 )
			err = MUJOE_ERR_FAILURE;
	}



	hrtim_timx_out_cfg_t *pOutCfg;
	for( uint8_t i = 0; i < HRTIM_TIMX_NUM_OUT_CH; i++ )
	{
		pOutCfg = &pCfg->outCfg[i];

		if( pOutCfg->enabled )
		{
			// Set output SET sources
			LL_HRTIM_OUT_SetOutputSetSrc(HRTIM, pOutCfg->Output, pOutCfg->setSrcs);
			// Set output RESET sources
			LL_HRTIM_OUT_SetOutputResetSrc(HRTIM, pOutCfg->Output, pOutCfg->rstSrcs);

			// Set polarity
			LL_HRTIM_OUT_SetPolarity( HRTIM,
									  pOutCfg->Output,
									  pOutCfg->positivePolarity ?
									  LL_HRTIM_OUT_POSITIVE_POLARITY : LL_HRTIM_OUT_NEGATIVE_POLARITY );
			// Enable the output
			LL_HRTIM_EnableOutput(HRTIM, pOutCfg->Output);
			if( LL_HRTIM_IsEnabledOutput( HRTIM, pOutCfg->Output ) == 0 )
			{
				err = MUJOE_ERR_FAILURE;
				break;
			}


		}

	}

	// Enable/disable preload
	if( pCfg->preloadEnabled )
		LL_HRTIM_TIM_EnablePreload(HRTIM, timer);
	else
		LL_HRTIM_TIM_DisablePreload(HRTIM, timer);

	err |= LL_HRTIM_TIM_IsEnabledPreload(HRTIM, timer ) == pCfg->preloadEnabled ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	return err;
}// init_hrtim_timx

static mujoe_err_t init_master_tim( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	HRTIM_TypeDef *HRTIM = ctx->hrtim.hal_hdl->Instance;

	uint32_t syncSrc = LL_HRTIM_SYNCOUT_SRC_TIMA_START;
	uint32_t syncCfg = LL_HRTIM_SYNCOUT_POSITIVE_PULSE;
	// Configure the sync output of the HRTIM
	// Refer to pg.970 of reference manual
	// Positive pulse (16xf_hrtim cycles width) on TIMA reset generated on hrtim_out_sync2
	// External gp timer TIM2 is operating in master/slave mode. The slave side of TIM2
	// is synchronized to hrtim_out_sync2 (Slave Reset mode). TIMA causes the hrtim_out_sync2 pulse
	// generation. Therefore, TIM2 is synchronized to HRTIM TIMA
	LL_HRTIM_ConfigSyncOut(HRTIM, syncCfg, syncSrc);

	// Force update on timer
	mujoe_hrtim_TIMx_forceUpdate(&ctx->hrtim, LL_HRTIM_TIMER_MASTER );

	// Validate settings
	err |= LL_HRTIM_GetSyncOutConfig(HRTIM) == syncCfg ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	err |= LL_HRTIM_GetSyncOutSrc(HRTIM) == syncSrc ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	return err;
}// init_master_tim

static mujoe_err_t config_zcd_feedback_blanking( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Compute the max starting period (min starting frequency) to be tracked (in timer cnt)
	uint32_t TIME_CMP3 = (uint32_t)((float)ctx->rt.start_period * ( 1.0f + ctx->cfg.startFreq_maxNegDev_percent/100.0f));

	// Compute the min starting period (max starting frequency) to be tracked (in timer cnt)
	uint32_t TIME_CMP2 = (uint32_t)((float)ctx->rt.start_period * ( 1.0f - ctx->cfg.startFreq_maxPosDev_percent/100.0f));

	// Clamp to 16 bit width of timer
	if( TIME_CMP2 > 65535u )
	{
		TIME_CMP2 = 65535u;
	}

	// Clamp to 16 bit width of timer
	if( TIME_CMP3 > 65535u )
	{
		TIME_CMP3 = 65535u;
	}

	// Ensure that Compare 3 is greater than Compare 2, otherwise abort
	// Bounds must make sense: Max frequency -> lower period must be CMP2; Min frequency -> higher period must be CMP3
	if( TIME_CMP3 < TIME_CMP2 )
	{
		return MUJOE_ERR_INV_PARAM;
	}

	// TIMF Output 2 (TF2) goes high between these two compare events...
	// this is used as a blanking signal for the TIM E EEV (zcd) capture (for frequency tracking)
	err |= mujoe_hrtim_TIMx_setCompareX( &ctx->hrtim, LL_HRTIM_TIMER_E, 2, TIME_CMP2);
	err |= mujoe_hrtim_TIMx_setCompareX( &ctx->hrtim, LL_HRTIM_TIMER_E, 3, TIME_CMP3);

	return err;
}// config_zcd_feedback_blanking

static mujoe_err_t config_pulseSkip_period( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	uint32_t TIMD_PER = (uint32_t)((float)ctx->rt.start_period * ( 1.0f + ctx->cfg.startFreq_maxNegDev_percent/100.0f));

	if( TIMD_PER > 65535u )
	{
		TIMD_PER = 65535u;
	}

	// set TIMD period to maximum possible period (lowest possible frequency),
	// this is used for pulse skipping
	err |= mujoe_hrtim_TIMx_set_period(&ctx->hrtim,
									   LL_HRTIM_TIMER_D,
									   TIMD_PER );

	return err;
}// config_pulseSkip_period

static mujoe_err_t config_masterRefTimer( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	// CMP1 is used for blanking (rejecting reset signals before the minimum period time elapsed)
	uint32_t TIMA_CMP1 = (uint32_t)((float)ctx->rt.start_period * ( 1.0f - ctx->cfg.startFreq_maxPosDev_percent/100.0f));

	if( TIMA_CMP1 > 65535u )
	{
		TIMA_CMP1 = 65535u;
	}

	// CMP2 is used for postpone (event is latched) eev9 (burst start) to
	// not interfere with period capture
	uint16_t TIMA_CMP2 = 5;

	err |= mujoe_hrtim_TIMx_setCompareX(&ctx->hrtim, LL_HRTIM_TIMER_A, 1, TIMA_CMP1);
	err |= mujoe_hrtim_TIMx_setCompareX(&ctx->hrtim, LL_HRTIM_TIMER_A, 2, TIMA_CMP2);

	return err;

}// config_masterRefTimer


static mujoe_err_t init_common( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Configure the HRTIM EEV module
	err |= config_eev_mapping( ctx  );

	// Init Master Period Reference Timer (TIM2)
	err |= init_tim2( ctx );

	// Init Burst generator timer (TIM5)
	err |= init_tim5( ctx );


	// Configure HRTIM Timers with initial settings
	for( uint8_t i = 0; i < HRTIM_NUM_TIMERS; i++ )
	{
		err |= init_hrtim_timx(ctx, hrtim_timx_cfg_tbl[i].timer, (hrtim_timx_cfg_t *)hrtim_timx_cfg_tbl[i].pCfg);
	}

	// Further configure each timer

	// TIM 2 - Lead time and Resonant Freq Injection
	{

		// Set lead time with TIM2
		err |= mujoe_resdrv_set_lead_time(ctx, ctx->cfg.leadTime_ns);
	}

	// TIMER A - Master Reference
	{
		// Configure master timer reference
		err |= config_masterRefTimer( ctx );
	}

	// TIMER B - Half Bridge 1 Driver
	{
		// blanking for rejecting tim2 trgo signals shortly after reset
		// should not happen anyway in principle but makes sure there is no reset from the signal delay hrtim->tim2->hrtim
		uint16_t timb_cmp1 = 5;
		err |= mujoe_hrtim_TIMx_setCompareX(&ctx->hrtim, LL_HRTIM_TIMER_B, 1, timb_cmp1);
	}

	// TIMER C - Half Bridge 2 Driver
	{
		// blanking for rejecting tim2 trgo signals shortly after reset
		// should not happen anyway in principle but makes sure there is no reset from the signal delay hrtim->tim2->hrtim
		uint16_t timc_cmp1 = 5;
		err |= mujoe_hrtim_TIMx_setCompareX( &ctx->hrtim, LL_HRTIM_TIMER_C, 1, timc_cmp1 );
	}

	// TIMER D - Pulse Skipping
	{
		// Configure pulse skipping timer period
		err |= config_pulseSkip_period( ctx );
	}

	// TIMER E - Frequency Tracking
	{

		// Configure the blanking window for ZCD feedback frequency tracking
		err |= config_zcd_feedback_blanking( ctx );


		// DMA request for lead time frequency tracking
		// Timer E Capture 1 DMA rq enable on hrtim_dma6 channel
		err |= mujoe_hrtim_captureDmaRequestControl( &ctx->hrtim,
													LL_HRTIM_TIMER_E,
													1,
													true);

	}

	// TIMER F - Current Feedback Timeout Detection
	{
		// Set ZCD feedback timeout (e.g. modify TIMF params)
		err |= set_zcdFeedback_timeout( ctx, ctx->cfg.zcdFeedback_timeout_us );
	}

	// MASTER TIMER
	{
		// Init master timer
		err |= init_master_tim( ctx );
	}

	// Force update on all timers (forces preload enabled timers to load SFRs with
	// shadow registers
	mujoe_hrtim_TIMx_forceUpdateAll(&ctx->hrtim);

	return err;
}// init_common

static inline mujoe_err_t config_resPerCapDma(RESDRV_CTX ctx)
{
	dma_cfg_t cfg =
	{
		.dmaChannel 		= LL_DMA_CHANNEL_7,
		.dmaRequestId 		= LL_DMAMUX_REQ_HRTIM1_E,				// DMA request MUX input = 100 (dec),				// DMA Request Mux input: 98 (dec) (hrtim_dma4)
		.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,	// DMA_CCRx.DIR = 0
		.SrcAddress 		= (uint32_t)&(HRTIM1_TIME->CPT1xR),		// HRTIM TIM E Capture 1
		.DstAddress 		= (uint32_t)ctx->pResPeriodCapBuff,
		.numberOfData 		= ctx->cfg.resPeriodCapBuffLen,			// Total number of data to transfer
		.periphSize 		= LL_DMA_PDATAALIGN_WORD,				// Size of data on peripheral side
		.memSize 			= LL_DMA_MDATAALIGN_WORD,				// Size of data on memory side
		// Lower prio relative to DMA channels that handle critical HRTIM operations
		.priority			= LL_DMA_PRIORITY_LOW,//LL_DMA_PRIORITY_MEDIUM,//LL_DMA_PRIORITY_HIGH,
		.circularModeEn		= true,
		.periphIncEn		= false,
		.memIncEn			= true,

	};

	return mujoe_dma_config(&ctx->dma2, &cfg);
}// config_resPerCapDma

static mujoe_err_t config_dma(  RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	if( ctx->cfg.enableBurstReload )
	{
		dma_cfg_t cfg =
		{
			.dmaChannel 		= LL_DMA_CHANNEL_6,
			.dmaRequestId 		= LL_DMAMUX_REQ_TIM5_CH2,
			.tranferDirection	= LL_DMA_DIRECTION_MEMORY_TO_PERIPH,	// DMA_CCRx.DIR = 1
			.SrcAddress 		= (uint32_t)&(ctx->rt.start_period),
			.DstAddress 		= (uint32_t)&(TIM2->ARR),		// TIM 2 ARR
			.numberOfData 		= 1,							// Total number of data to transfer
			.periphSize 		= LL_DMA_PDATAALIGN_WORD,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_WORD,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= true,
			.periphIncEn		= false,
			.memIncEn			= false,

		};

		err |= mujoe_dma_config(&ctx->dma2, &cfg);
	}

	// DMA configuration for lead time frequency tracking
	{
		dma_cfg_t cfg =
		{
			.dmaChannel 		= LL_DMA_CHANNEL_1,
			.dmaRequestId 		= LL_DMAMUX_REQ_HRTIM1_E,	// DMA request MUX input = 100 (dec)
			.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,	// DMA_CCRx.DIR = 0
			.SrcAddress 		= (uint32_t)&(HRTIM1_TIME->CPT1xR),			// HRTIM TIM E Capture 1
			.DstAddress 		= (uint32_t)&(TIM2->ARR),					// TIM 2 ARR
			.numberOfData 		= 1,							// Total number of data to transfer
			.periphSize 		= LL_DMA_PDATAALIGN_WORD,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_WORD,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= true,
			.periphIncEn		= false,
			.memIncEn			= false,

		};

		err |= mujoe_dma_config(&ctx->dma2, &cfg);
	}

	// DMA config for exiting delayed idle state within the delayed protection system
	// After interrupter pulse end (TIM5 output pulse falling edge on COMP4 input), HRTIM TIMB and TIMC
	// outputs enter delayed idle in indefinitely. We use TIM5 update (next pulse) to re-enable the
	// timer outputs: TB1, TB2, TC1, and TC2 via a DMA request.
	// Refer to Delayed Protection section of reference manual (pg. 935)
	{
		dma_cfg_t cfg =
		{
			.dmaChannel 		= LL_DMA_CHANNEL_5,
			.dmaRequestId 		= LL_DMAMUX_REQ_TIM5_UP,
			.tranferDirection	= LL_DMA_DIRECTION_MEMORY_TO_PERIPH,	// DMA_CCRx.DIR = 1
			.SrcAddress 		= (uint32_t)&(OENR_ENABLE_BRIDGE_DRV_OUT),
			.DstAddress 		= (uint32_t)&(HRTIM1_COMMON->OENR),		// HRTIM OENR Register (Output Enable Register)
			.numberOfData 		= 1,							// Total number of data to transfer
			.periphSize 		= LL_DMA_PDATAALIGN_WORD,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_WORD,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= true,
			.periphIncEn		= false,
			.memIncEn			= false,

		};

		err |= mujoe_dma_config(&ctx->dma2, &cfg);
	}

#if TIMB_TIMC_CMP2_DMA_EN==1
	{

		// Timer B Capture 1 DMA rq enable
		err |= mujoe_hrtim_captureDmaRequestControl( &ctx->hrtim,
													LL_HRTIM_TIMER_B,
													1,
													true);

		dma_cfg_t ch7_cfg =
		{
			.dmaChannel 		= LL_DMA_CHANNEL_7,
			.dmaRequestId 		= LL_DMAMUX_REQ_HRTIM1_B,	// DMA request MUX input = 100 (dec)
			.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,	// DMA_CCRx.DIR = 0
			.SrcAddress 		= (uint32_t)&(HRTIM1_TIMB->CPT1xR),			// HRTIM TIM B Capture 1
			.DstAddress 		= (uint32_t)&(HRTIM1_TIMB->CMP2xR),			// HRTIM TIM B Compare 2
			.numberOfData 		= 1,							// Total number of data to transfer
			.periphSize 		= LL_DMA_PDATAALIGN_WORD,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_WORD,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= true,
			.periphIncEn		= false,
			.memIncEn			= false,

		};

		err |= mujoe_dma_config(&ctx->dma2, &ch7_cfg);
	}
#endif // #if TIMB_TIMC_CMP2_DMA_EN==1

	// TODO: Remove config not related to pulse skipping outside of conditional (e.g. burst config etc)
	if(ctx->cfg.enablePulseSkipping )
	{
		/////////////////////////////////////////////////////////////////////
		// DMA2 CH4: Update HRTIM TIMD compare with TIME capture value
		////////////////////////////////////////////////////////////////////
		// HRTIM TIM C Capture 2 triggers DMA request
		{
			dma_cfg_t cfg =
			{
				.dmaChannel 		= LL_DMA_CHANNEL_4,
				.dmaRequestId 		= LL_DMAMUX_REQ_HRTIM1_C,				// DMA Request Mux input: 98 (dec) (hrtim_dma4)
				.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,	// DMA_CCRx.DIR = 0
				.SrcAddress 		= (uint32_t)&(HRTIM1_TIME->CPT1xR),		// HRTIM TIM E Capture 1
				.DstAddress 		= (uint32_t)&(HRTIM1_TIMD->CMP1xR),		// HRTIM TIM D Compare 1
				.numberOfData 		= 1,									// Total number of data to transfer
				.periphSize 		= LL_DMA_PDATAALIGN_WORD,				// Size of data on peripheral side
				.memSize 			= LL_DMA_MDATAALIGN_WORD,				// Size of data on memory side
				.priority			= LL_DMA_PRIORITY_HIGH,
				.circularModeEn		= true,
				.periphIncEn		= false,
				.memIncEn			= false,

			};

			err |= mujoe_dma_config(&ctx->dma2, &cfg);

		}

		// Refer to HRTIM DMA Burst Section (pg. 985)
		//It is only necessary to have the DMA controller pointing to the HRTIM_BDMADR register as
		//the destination, in the memory, to the peripheral configuration with the peripheral increment
		//mode disabled (the HRTIM handles internally the data re-routing to the final destination
		//register).
		{
			// Set update gating for DMA burst operation for TIMB
			// HRTIM_TIMBCR.UPDGAT = 0x2
			// The update occurs on the update event following the DMA burst transfer completion
			err |= mujoe_hrtim_setUpdateGating(&ctx->hrtim,
												LL_HRTIM_TIMER_B,
												LL_HRTIM_UPDATEGATING_DMABURST_UPDATE);

			// Set update gating for DMA burst operation for TIMC
			// The update occurs on the update event following the DMA burst transfer completion
			err |= mujoe_hrtim_setUpdateGating(&ctx->hrtim,
												LL_HRTIM_TIMER_C,
												LL_HRTIM_UPDATEGATING_DMABURST_UPDATE);


		    // Configure DMA burst for HRTIM TIMB
			mujoe_hrtim_configBurstDMA( &ctx->hrtim,
										LL_HRTIM_TIMER_B,
										LL_HRTIM_BURSTDMA_TIMSET1R |
										LL_HRTIM_BURSTDMA_TIMSET2R );

			// Configure DMA burst for HRTIM TIMC
			mujoe_hrtim_configBurstDMA( &ctx->hrtim,
										LL_HRTIM_TIMER_C,
										LL_HRTIM_BURSTDMA_TIMSET1R |
										LL_HRTIM_BURSTDMA_TIMSET2R );

			// Configure DMA requests for pulse skipping method 2
			{
				// Timer B Capture 2 DMA rq enable on hrtim_dma3 channel
				err |= mujoe_hrtim_captureDmaRequestControl( &ctx->hrtim,
															LL_HRTIM_TIMER_B,
															2,
															true);

				// Timer C Capture 2 DMA rq enable on hrtim_dma4 channel
				err |= mujoe_hrtim_captureDmaRequestControl( &ctx->hrtim,
															LL_HRTIM_TIMER_C,
															2,
															true);

				// Timer D Compare 1 DMA rq enable on hrtim_dma5 channel
				err |= mujoe_hrtim_compareDmaRequestControl( &ctx->hrtim,
															LL_HRTIM_TIMER_D,
															1,
															true );
			}


			// DMA2 CH2: HRTIM TIM D triggers DMA Burst
			// HRTIM TIM B Capture 2 triggers DMA request
			{
				hrtim_dma_burst_cfg_t cfg =
				{
					.DMA 				= ctx->dma2.instance,
					.dmaChannel 		= LL_DMA_CHANNEL_2,
					.dmaRequestId		= LL_DMAMUX_REQ_HRTIM1_B,		// DMA Request MUX Input: 97 (dec) (hrtim_dma3)
					.SrcAddress 		= (uint32_t)(output_reset),
					.numRegisters 		= PULSE_SKIP_OUT_RESET_LEN,
				    .circularModeEn 	= true,
					.priority 			= LL_DMA_PRIORITY_HIGH,
					.enableInterrupts 	= ctx->cfg.enableDMACallbacks,
				};
				err |= mujoe_hrtim_dma_burst_config( &ctx->hrtim, &cfg);
			}

			// DMA2 CH3: HRTIM TIM D triggers DMA Burst
			// HRTIM TIM D Compare 1 triggers DMA request
			{

				hrtim_dma_burst_cfg_t cfg =
				{
					.DMA 				= ctx->dma2.instance,
					.dmaChannel 		= LL_DMA_CHANNEL_3,
					.dmaRequestId		= LL_DMAMUX_REQ_HRTIM1_D,	// DMA Request MUX Input: 99 (dec) (hrtim_dma5)
					.SrcAddress 		= (uint32_t)(output_set),
					.numRegisters 		= PULSE_SKIP_OUT_SET_LEN,
				    .circularModeEn 	= true,
					.priority 			= LL_DMA_PRIORITY_HIGH,
					.enableInterrupts 	= ctx->cfg.enableDMACallbacks,
				};
				err |= mujoe_hrtim_dma_burst_config( &ctx->hrtim, &cfg);

			}

		}
	} // if( ctx->cfg.enablePulseSkipping )

	// Configure DMA for TIME.CPT1ER captures if enabled via config
	if( ctx->cfg.resPeriodCapBuffLen && ctx->pResPeriodCapBuff != NULL )
	{
		err |= config_resPerCapDma(ctx);
	}

	return err;
}// config_dma


static mujoe_err_t config_gpio( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// HRTIM TIMF TF2 - PC7
	if( ctx->cfg.dbg_cfg.enable_TF2OutputPin )
	{
		  GPIO_TypeDef *PORT = GPIOC;
		  uint32_t pin = LL_GPIO_PIN_7;

		  LL_GPIO_ResetOutputPin(PORT, pin);

		  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
		  GPIO_InitStruct.Pin = pin;
		  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
		  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
		  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
		  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
		  GPIO_InitStruct.Alternate = LL_GPIO_AF_13;
		  ErrorStatus stat = LL_GPIO_Init(PORT, &GPIO_InitStruct);
		  err = stat == SUCCESS ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}
	else
		err |= mujoe_gpio_set_inputPullup( GPIOC, LL_GPIO_PIN_7 );

	return err;

}// config_gpio

static mujoe_err_t init_hrtim( RESDRV_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// Initialize common components within/without HRTIM
	err |= init_common(ctx);

	// Configure DMA channels
	err |= config_dma(ctx);

	// Configure GPIO as it pertains to HRTIM
	err |= config_gpio(ctx);

	// Reset all HRTIM timer counters
	mujoe_hrtim_TIMx_counter_reset_all( &ctx->hrtim );

	return err;
}// init_hrtim

static bool isStateChangeValid( RESDRV_CTX ctx, resdrv_sys_state_t newState )
{
	 bool valid = false;

	 switch( ctx->rt.sys_state )
	 {
	 	 case RESDRV_SYS_STATE_INIT:
	 		 if( newState == RESDRV_SYS_STATE_IDLE_DISARMED )
			 {
				 valid = true;
			 }
	 		 break;
	 	 case RESDRV_SYS_STATE_IDLE_DISARMED:
	 		 if( newState == RESDRV_SYS_STATE_IDLE_ARMED)
			 {
				 valid = true;
			 }
	 		 break;
	 	 case RESDRV_SYS_STATE_IDLE_ARMED:
	 		 if( (newState == RESDRV_SYS_STATE_FAULT) ||
	 			 (newState == RESDRV_SYS_STATE_IDLE_DISARMED) ||
				 (newState == RESDRV_SYS_STATE_RUN) )
	 		 {
	 			 valid = true;
	 		 }
	 		 break;
	 	 case RESDRV_SYS_STATE_RUN:
	 		 if( (newState == RESDRV_SYS_STATE_FAULT) ||
	 			 (newState == RESDRV_SYS_STATE_IDLE_ARMED) )
	 		 {
	 			 valid = true;
	 		 }
	 		 break;
	 	 case RESDRV_SYS_STATE_FAULT:
	 		 if( newState == RESDRV_SYS_STATE_IDLE_DISARMED )
	 		 {
	 			 valid = true;
	 		 }
	 		 break;
	 	 default:
	 		 break;
	 }

	 return valid;
}// isStateChangeValid

////////////////////////////////////////////////////////////////////////
// CALLBACKS
////////////////////////////////////////////////////////////////////////

// If 1, only DMA channels related to pulse skipping are logged
#define PULSE_SKIP_LOG_ONLY_EN			0

static inline void handle_finite_cycles( void )
{
	if( _ctx->cfg.numBurstCycles )
	{
		if( ++_ctx->rt.prfCycleCnt == _ctx->cfg.numBurstCycles )
		{
			if( _ctx->pCbs && _ctx->pCbs->numPrfCyclesCompleteCB )
			{
				_ctx->pCbs->numPrfCyclesCompleteCB(NULL);
			}
		}
	}
}// handle_finite_cycles

static void dma2TransferComplete_CB(uint8_t dmaCh)
{

	switch( dmaCh )
	{

#if PULSE_SKIP_LOG_ONLY_EN == 0
		case 1:
			break;
		case 5:
			break;
		case 6:
			// If number of PRF cycle(s) is finite: check if target number of
			// PRF cycles has been reached
			handle_finite_cycles();
			break;
#endif // #if PULSE_SKIP_LOG_ONLY_EN == 0
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		default:
			break;
	}

}// dma2TransferComplete_CB

static void dma2TransferError_CB(uint8_t dmaCh)
{

#if RES_DRV_DBG_EN == 1
	LOG_ERROR("dma2TransferError_CB:: DMA2 CH%d XFR error!",dmaCh);
#endif // #if RES_DRV_DBG_EN == 1

}// dma2TransferError_CB

////////////////////////////////////////////////////////////////////////
// ISR
////////////////////////////////////////////////////////////////////////

#if RES_DRV_DBG_EN == 1

volatile uint32_t tima_cmpcnt = 0, time_cmpcnt = 0;

#endif // #if RES_DRV_DBG_EN == 1

void HRTIM1_TIMA_IRQHandler(void)
{
	// COMPARE 4
    if (LL_HRTIM_IsActiveFlag_CMP4(HRTIM1, LL_HRTIM_TIMER_A))
    {
        LL_HRTIM_ClearFlag_CMP4(HRTIM1, LL_HRTIM_TIMER_A);
        tima_cmpcnt++;
    }
}// HRTIM1_TIMA_IRQHandler

void HRTIM1_TIME_IRQHandler(void)
{
    // Check if the capture 1 interrupt flag for Timer E is set
    if (LL_HRTIM_IsActiveFlag_CPT1(HRTIM1, LL_HRTIM_TIMER_E))
    {
        // Clear the interrupt flag to prevent re-entry.
        LL_HRTIM_ClearFlag_CPT1(HRTIM1, LL_HRTIM_TIMER_E);
    }

    if (LL_HRTIM_IsActiveFlag_CMP4(HRTIM1, LL_HRTIM_TIMER_E))
    {
        LL_HRTIM_ClearFlag_CMP4(HRTIM1, LL_HRTIM_TIMER_E);
        time_cmpcnt++;
    }
}// HRTIM1_TIME_IRQHandler

