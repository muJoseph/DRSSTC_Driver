/*
 * mujoe_adc.c
 *
 *  Created on: Nov 9, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_adc.h"
#include <string.h>
#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

/* Timeout values for ADC operations. */
/* (calibration, enable settling time, disable settling time, ...)          */
/* Values defined to be higher than worst cases: low clock frequency,       */
/* maximum prescalers.                                                      */
/* Example of profile very low frequency : ADC clock frequency 0.14MHz      */
/* prescaler 256, sampling time 247.5 ADC clock cycles, resolution 12 bits. */
/*  - ADC calibration time: On STM32G4 ADC, maximum delay is 112/fADC,      */
/*    resulting in a maximum delay of 800us                                 */
/*    (refer to device datasheet, parameter "tCAL")                         */
/*  - ADC enable time: maximum delay is 1 conversion cycle.                 */
/*    (refer to device datasheet, parameter "tSTAB")                        */
/*  - ADC disable time: maximum delay should be a few ADC clock cycles      */
/*  - ADC stop conversion time: maximum delay should be a few ADC clock     */
/*    cycles                                                                */
/*  - ADC conversion time: with this hypothesis of clock settings, maximum  */
/*    delay will be 476ms.                                                  */
/*    (refer to device reference manual, section "Timing")                  */
/* Unit: ms                                                                 */
#define ADC_CALIBRATION_TIMEOUT_MS       (   1U)
#define ADC_ENABLE_TIMEOUT_MS            (   1U)
#define ADC_DISABLE_TIMEOUT_MS           (   1U)
#define ADC_STOP_CONVERSION_TIMEOUT_MS   (   1U)
#define ADC_CONVERSION_TIMEOUT_MS        ( 500U)

/* Delay between ADC end of calibration and ADC enable.                     */
/* Delay estimation in CPU cycles: Case of ADC enable done                  */
/* immediately after ADC calibration, ADC clock setting slow                */
/* (LL_ADC_CLOCK_ASYNC_DIV32). Use a higher delay if ratio                  */
/* (CPU clock / ADC clock) is above 32.                                     */
#define ADC_DELAY_CALIB_ENABLE_CPU_CYCLES  (LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES * 32)


/* Definitions of data related to this example */
  /* Full-scale digital value with a resolution of 12 bits (voltage range     */
  /* determined by analog voltage references Vref+ and Vref-,                 */
  /* refer to reference manual).                                              */
#define DIGITAL_SCALE_12BITS             (0xFFFUL)

#define ADC_NUM_BIT						 12

/* Init variable out of ADC expected conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE    (DIGITAL_SCALE_12BITS + 1)

/* Gain compensation x1 factor */
#define GAIN_COMPENSATION_X1_FACTOR      (0x1000UL)


#ifndef VDDA_APPLI
  /* Value of analog reference voltage (Vref+), connected to analog voltage   */
  /* supply Vdda (unit: mV).                                                  */
#define VDDA_APPLI                       ((uint32_t)3300)
#endif // VDDA_APPLI



#define GAIN_COMPENSATION_VAL	 		VDDA_APPLI * GAIN_COMPENSATION_X1_FACTOR / DIGITAL_SCALE_12BITS
////////////////////////////////////////////////////////////////////////
// LOCAL VAR
////////////////////////////////////////////////////////////////////////

__IO uint16_t uhADC2GrpInjectedConvertedData = VAR_CONVERTED_DATA_INIT_VALUE;

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_adc_initContext( ADC_CTX ctx, adc_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(adc_ctx_t));
	ctx->cfg = *pCfg;
	return MUJOE_ERR_NONE;

}// mujoe_adc_initContext


uint32_t mujoe_adc_codeToVdiff_mv(ADC_CTX ctx, uint32_t adcCode_diff )
{
	// Vdiff = Vinp - Vinn = ((code * (2/full_scale)) - 1) * Vref+
//	uint32_t vdif_mv = adcCode_diff << 1;	// mul by 2
//	vdif_mv >>= ADC_NUM_BIT;		// divide by 4096
//	vdif_mv -= 1;
//	vdif_mv *= 1e3;	// convert to mv
	//return vdif_mv;

	float vdif_mv = (((float)adcCode_diff * 2.0f/(float)DIGITAL_SCALE_12BITS) - 1.0f)*(float)VDDA_APPLI;
	return (uint32_t)vdif_mv;

}// mujoe_adc_codeToVdiff_mv

uint32_t mujoe_adc_ReadRegularConversionData32(ADC_CTX ctx)
{
	return LL_ADC_REG_ReadConversionData32(ctx->cfg.instance);
}// mujoe_adc_ReadRegularConversionData32

mujoe_err_t mujoe_adc_setChSamplingTime( ADC_CTX ctx, uint32_t Channel, uint32_t SamplingTime )
{
	LL_ADC_SetChannelSamplingTime(ctx->cfg.instance, Channel, SamplingTime);
	return LL_ADC_GetChannelSamplingTime(ctx->cfg.instance, Channel) == SamplingTime ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_adc_setChSamplingTime

static mujoe_err_t initCh( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	ADC_TypeDef *adc = ctx->cfg.instance;

	// Configure Regular Channel
	// Map Channel 2 to Rank 1
	LL_ADC_REG_SetSequencerRanks(adc, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_2);
	err |= mujoe_adc_setChSamplingTime( ctx, LL_ADC_CHANNEL_2,LL_ADC_SAMPLINGTIME_12CYCLES_5);
	//LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_2, LL_ADC_SAMPLINGTIME_2CYCLES_5);
	LL_ADC_SetChannelSingleDiff(adc, LL_ADC_CHANNEL_2, LL_ADC_DIFFERENTIAL_ENDED);

	// Configure Injected Channel
	// Map ADCx Channel 14 to injected rank 1
	LL_ADC_INJ_SetSequencerRanks(adc, LL_ADC_INJ_RANK_1, LL_ADC_CHANNEL_14);
	err |= mujoe_adc_setChSamplingTime( ctx, LL_ADC_CHANNEL_14,LL_ADC_SAMPLINGTIME_12CYCLES_5);
	//LL_ADC_SetChannelSamplingTime(adc, LL_ADC_CHANNEL_14, LL_ADC_SAMPLINGTIME_2CYCLES_5);
	LL_ADC_SetChannelSingleDiff(adc, LL_ADC_CHANNEL_14, LL_ADC_SINGLE_ENDED);

	return err;

}// initCh

static mujoe_err_t initCommon( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	ADC_TypeDef *adc = ctx->cfg.instance;
	ErrorStatus stat = SUCCESS;
	LL_ADC_InitTypeDef ADC_InitStruct = {0};
	LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
	LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
	LL_ADC_INJ_InitTypeDef ADC_INJ_InitStruct = {0};

	ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
	ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
	ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
	stat |= LL_ADC_Init(adc, &ADC_InitStruct);
	ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
	ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
	ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
	ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
	ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
	ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
	stat |= LL_ADC_REG_Init(adc, &ADC_REG_InitStruct);
	LL_ADC_SetGainCompensation(adc, ctx->cfg.gainCompensationEn ?
									GAIN_COMPENSATION_VAL :
									0 );
	err |= (LL_ADC_GetGainCompensation(adc) == (ctx->cfg.gainCompensationEn ? GAIN_COMPENSATION_VAL : 0) ) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	LL_ADC_SetOverSamplingScope(adc, LL_ADC_OVS_DISABLE);
	ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
	stat |= LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(adc), &ADC_CommonInitStruct);
	ADC_INJ_InitStruct.TriggerSource = LL_ADC_INJ_TRIG_SOFTWARE;
	ADC_INJ_InitStruct.SequencerLength = LL_ADC_INJ_SEQ_SCAN_DISABLE;
	ADC_INJ_InitStruct.SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_DISABLE;
	ADC_INJ_InitStruct.TrigAuto = LL_ADC_INJ_TRIG_INDEPENDENT;
	stat |= LL_ADC_INJ_Init(adc, &ADC_INJ_InitStruct);
	LL_ADC_INJ_SetQueueMode(adc, LL_ADC_INJ_QUEUE_DISABLE);
	return ((stat == SUCCESS) && (err == MUJOE_ERR_NONE )) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// initCommon


static inline IRQn_Type getIrq( ADC_CTX ctx  )
{
	ADC_TypeDef *adc = ctx->cfg.instance;
	if( (adc == ADC1) || (adc == ADC2) )
	{
		return ADC1_2_IRQn;
	}
	else
	{
		TRAP_MCU();	// TODO: Implement
		return ADC3_IRQn;
	}

}// getIrq

static mujoe_err_t initInt( ADC_CTX ctx  )
{
	if( ctx->cfg.intEn )
	{
		IRQn_Type irqType = getIrq(ctx);
		// Clear pending interrupts
		NVIC_ClearPendingIRQ(irqType);

		// Configure interrupt priority
#if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
		// Must be of lower priority than configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (numerically higher)
		NVIC_SetPriority(irqType, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1 );
#else
		NVIC_SetPriority(irqType, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
#endif // #if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)

		NVIC_EnableIRQ(irqType);
	}


	return MUJOE_ERR_NONE;
}// initInt

  /**
  * @brief  Perform ADC activation procedure to make it ready to convert
  *         (ADC instance: ADC1).
  * @note   Operations:
  *         - ADC instance
  *           - Disable deep power down
  *           - Enable internal voltage regulator
  *           - Run ADC self calibration
  *           - Enable ADC
  *         - ADC group regular
  *           none: ADC conversion start-stop to be performed
  *                 after this function
  *         - ADC group injected
  *           none: ADC conversion start-stop to be performed
  *                 after this function
  * @param  None
  * @retval None
  */
mujoe_err_t mujoe_adc_initHardware( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	ADC_TypeDef *adc = ctx->cfg.instance;

  __IO uint32_t wait_loop_index = 0U;

  /*## Operation on ADC hierarchical scope: ADC instance #####################*/

  /* Note: Hardware constraint (refer to description of the functions         */
  /*       below):                                                            */
  /*       On this STM32 series, setting of these features is conditioned to   */
  /*       ADC state:                                                         */
  /*       ADC must be disabled.                                              */
  /* Note: In this example, all these checks are not necessary but are        */
  /*       implemented anyway to show the best practice usages                */
  /*       corresponding to reference manual procedure.                       */
  /*       Software can be optimized by removing some of these checks, if     */
  /*       they are not relevant considering previous settings and actions    */
  /*       in user application.
  */
  // Ensure that ADC is disabled
  if (LL_ADC_IsEnabled(adc) == 0)
  {

	// Configure interrupts
	err |= initInt(ctx);
	// Configure common features
	err |= initCommon(ctx);
	// Initialize channels
	err |= initCh(ctx);

    /* Disable ADC deep power down (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(adc);

    /* Enable ADC internal voltage regulator */
    LL_ADC_EnableInternalRegulator(adc);

    /* Delay for ADC internal voltage regulator stabilization.                */
    /* Compute number of CPU cycles to wait for, from delay in us.            */
    /* Note: Variable divided by 2 to compensate partially                    */
    /*       CPU processing cycles (depends on compilation optimization).     */
    /* Note: If system core clock frequency is below 200kHz, wait time        */
    /*       is only a few CPU processing cycles.                             */
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }

    // Run adc calibration for single ended
    LL_ADC_StartCalibration(adc, LL_ADC_SINGLE_ENDED);

    /* Poll for ADC effectively calibrated */
    while (LL_ADC_IsCalibrationOnGoing(adc) != 0)
    {
    }

    // Run adc calibration for differential
    LL_ADC_StartCalibration(adc, LL_ADC_DIFFERENTIAL_ENDED);

    /* Poll for ADC effectively calibrated */
    while (LL_ADC_IsCalibrationOnGoing(adc) != 0)
    {
    }

    /* Delay between ADC end of calibration and ADC enable.                   */
    /* Note: Variable divided by 2 to compensate partially                    */
    /*       CPU processing cycles (depends on compilation optimization).     */
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }

    // Configure conversion mode
    uint32_t contMode = ctx->cfg.continuous ? LL_ADC_REG_CONV_CONTINUOUS : LL_ADC_REG_CONV_SINGLE;
    LL_ADC_REG_SetContinuousMode(adc, contMode);
    err |= LL_ADC_REG_GetContinuousMode( adc ) == contMode ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

    // Enable DMA tranfer mode if neccessary
    if( ctx->cfg.dmaCfg.enable )
    {
    	uint32_t DMATransfer = ctx->cfg.continuous ? LL_ADC_REG_DMA_TRANSFER_UNLIMITED : LL_ADC_REG_DMA_TRANSFER_LIMITED;
    	LL_ADC_REG_SetDMATransfer(adc, DMATransfer);
    	err |= LL_ADC_REG_GetDMATransfer(adc) == DMATransfer ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
    }

    // Clear ready flag, enable adc and wait for ready state
    LL_ADC_ClearFlag_ADRDY(adc);
    LL_ADC_Enable(adc);
    if( LL_ADC_IsEnabled(adc) == 0 )
    {
    	return MUJOE_ERR_NONE;
    }
    while (LL_ADC_IsActiveFlag_ADRDY(adc) == 0)
    {

    }

    /* Note: ADC flag ADRDY is not cleared here to be able to check ADC       */
    /*       status afterwards.                                               */
    /*       This flag should be cleared at ADC Deactivation, before a new    */
    /*       ADC activation, using function "LL_ADC_ClearFlag_ADRDY()".       */
  }

  /*## Operation on ADC hierarchical scope: ADC group regular ################*/
  /* Note: No operation on ADC group regular performed here.                  */
  /*       ADC group regular conversions to be performed after this function  */
  /*       using function:                                                    */
  /*       "LL_ADC_REG_StartConversion();"                                    */

  /*## Operation on ADC hierarchical scope: ADC group injected ###############*/
  /* Note: No operation on ADC group injected performed here.                 */
  /*       ADC group injected conversions to be performed after this function */
  /*       using function:                                                    */
  /*       "LL_ADC_INJ_StartConversion();"                                    */

  return err;

}// mujoe_adc_initHardware

static mujoe_err_t config_dma( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	uint32_t dmaRequestId;
	ADC_TypeDef *adc = ctx->cfg.instance;

	if( adc == ADC2 )
	{
		dmaRequestId = LL_DMAMUX_REQ_ADC2;
	}
	else
	{
		return MUJOE_ERR_NOT_IMPLEMENTED;
	}

	dma_cfg_t cfg =
	{
		.dmaChannel 		= ADC2_DMA_CH,
		.dmaRequestId 		= dmaRequestId,
		.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
		.SrcAddress 		= (uint32_t)&(ctx->cfg.instance->DR),	// ADC data register
		//.DstAddress 		= (uint32_t)&(ctx->cfg.dmaCfg.pSampleBuffer),		// Data buffer
		.DstAddress 		= (uint32_t)(ctx->cfg.dmaCfg.pSampleBuffer),		// Data buffer
		.numberOfData 		= 1,							// Total number of data to transfer
		.periphSize 		= LL_DMA_PDATAALIGN_WORD,		// Size of data on peripheral side
		.memSize 			= LL_DMA_MDATAALIGN_WORD,		    // Size of data on memory side
		.priority			= LL_DMA_PRIORITY_HIGH,
		.circularModeEn		= ctx->cfg.continuous,	// If ADC continuous mode, DMA must be circular
		.periphIncEn		= false,
		.memIncEn			= false,

	};

	err |= mujoe_dma_config_lite(ctx->cfg.dmaCfg.instance, &cfg, ctx->cfg.dmaCfg.enDmaInt );

	return err;

}// config_dma




mujoe_err_t mujoe_adc_startInjectedConversion( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	ADC_TypeDef *adc = ctx->cfg.instance;

	// Ensure that no injected conversion is ongoing
	if( LL_ADC_INJ_IsConversionOngoing(adc) )
	{
	  return MUJOE_ERR_FAILURE;
	}

	// Start conversion
	LL_ADC_INJ_StartConversion(adc);
    uint32_t ongoing = LL_ADC_INJ_IsConversionOngoing(adc);
    err |= ongoing ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	return err;

}// mujoe_adc_startInjectedConversion

mujoe_err_t mujoe_adc_cntrlInt_RegEOC( ADC_CTX ctx, bool enable )
{
	if( !ctx->cfg.intEn )
	{
		return MUJOE_ERR_NOT_SUPPORTED;
	}

	ADC_TypeDef *adc = ctx->cfg.instance;
	if( enable )
	{
		LL_ADC_EnableIT_EOC(adc);
	}
	else
	{
		LL_ADC_DisableIT_EOC(adc);
	}

	return LL_ADC_IsEnabledIT_EOC(adc) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_adc_cntrlInt_RegEOC

mujoe_err_t mujoe_adc_cntrlInt_RegEOS( ADC_CTX ctx, bool enable )
{
	if( !ctx->cfg.intEn )
	{
		return MUJOE_ERR_NOT_SUPPORTED;
	}

	ADC_TypeDef *adc = ctx->cfg.instance;
	if( enable )
	{
		LL_ADC_EnableIT_EOS(adc);
	}
	else
	{
		LL_ADC_DisableIT_EOS(adc);
	}

	return LL_ADC_IsEnabledIT_EOS(adc) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_adc_cntrlInt_RegEOS

mujoe_err_t mujoe_adc_cntrlInt_InjEOC( ADC_CTX ctx, bool enable )
{
	if( !ctx->cfg.intEn )
	{
		return MUJOE_ERR_NOT_SUPPORTED;
	}

	ADC_TypeDef *adc = ctx->cfg.instance;
	if( enable )
	{
		LL_ADC_EnableIT_JEOC(adc);
	}
	else
	{
		LL_ADC_DisableIT_JEOC(adc);
	}

	return LL_ADC_IsEnabledIT_JEOC(adc) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_adc_cntrlInt_InjEOC

mujoe_err_t mujoe_adc_cntrlInt_InjEOS( ADC_CTX ctx, bool enable )
{
	if( !ctx->cfg.intEn )
	{
		return MUJOE_ERR_NOT_SUPPORTED;
	}

	ADC_TypeDef *adc = ctx->cfg.instance;
	if( enable )
	{
		LL_ADC_EnableIT_JEOS(adc);
	}
	else
	{
		LL_ADC_DisableIT_JEOS(adc);
	}

	return LL_ADC_IsEnabledIT_JEOS(adc) == enable ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_adc_cntrlInt_InjEOS

mujoe_err_t mujoe_adc_startConversion( ADC_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	ADC_TypeDef *adc = ctx->cfg.instance;

	// Ensure that no conversion is ongoing
	if( LL_ADC_REG_IsConversionOngoing(adc) )
	{
	  return MUJOE_ERR_FAILURE;
	}

	if( ctx->cfg.dmaCfg.enable )
	{
		err |= config_dma( ctx );
	}

	// Start conversion
    LL_ADC_REG_StartConversion(adc);
    uint32_t ongoing = LL_ADC_REG_IsConversionOngoing(adc);
    err |= ongoing ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	return err;

}// mujoe_adc_startConversion


/**
  * @brief  ADC group injected end of unitary conversion interruption callback
  * @note   This function is executed when the ADC group injected
  *         sequencer has converted one ranks of the sequence.
  * @retval None
  */
void mujoe_adc2_grpInjUnitaryConvComplete_CB()
{
  // Retrieve incected ADC conversion data

  // NOTE: This requires gain compensation to be set correctly for result to be in mv, otherwise,
  // manual conversion is neccessary
  uhADC2GrpInjectedConvertedData = LL_ADC_INJ_ReadConversionData12(ADC2, LL_ADC_INJ_RANK_1);


}// mujoe_adc2_grpInjUnitaryConvComplete_CB
