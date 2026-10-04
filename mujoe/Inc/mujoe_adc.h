/*
 * mujoe_adc.h
 *
 *  Created on: Nov 9, 2025
 *      Author: Joe
 */

#ifndef MUJOE_ADC_H_
#define MUJOE_ADC_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "adc.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_dma.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

/* Definitions of data related to this example */
  /* Full-scale digital value with a resolution of 12 bits (voltage range     */
  /* determined by analog voltage references Vref+ and Vref-,                 */
  /* refer to reference manual).                                              */
#define DIGITAL_SCALE_12BITS             (0xFFFUL)

  /* Init variable out of ADC expected conversion data range */
#define VAR_CONVERTED_DATA_INIT_VALUE    (DIGITAL_SCALE_12BITS + 1)

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef struct _adc_dma_cfg
{
	bool			enable;
	DMA_TypeDef		*instance;
	uint32_t		*pSampleBuffer;
	bool			enDmaInt;	// NOTE: Only enable this for debug, do NOT use in prod, EXTREME CPU load

}adc_dma_cfg_t;

typedef struct _adc_cfg
{
	ADC_TypeDef		*instance;
	bool			gainCompensationEn;
	bool			differential;
	bool			continuous;
	adc_dma_cfg_t	dmaCfg;
	bool			intEn;

}adc_cfg_t;

typedef struct _adc_ctx
{
	adc_cfg_t  		cfg;

}adc_ctx_t, *ADC_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_adc_initContext( ADC_CTX ctx, adc_cfg_t *pCfg );
extern mujoe_err_t mujoe_adc_initHardware( ADC_CTX ctx );
extern mujoe_err_t mujoe_adc_startConversion( ADC_CTX ctx );
extern mujoe_err_t mujoe_adc_startInjectedConversion( ADC_CTX ctx );

extern uint32_t mujoe_adc_ReadRegularConversionData32(ADC_CTX ctx);
extern uint32_t mujoe_adc_codeToVdiff_mv(ADC_CTX ctx, uint32_t adcCode_diff );

extern mujoe_err_t mujoe_adc_setChSamplingTime( ADC_CTX ctx, uint32_t Channel, uint32_t SamplingTime );

extern mujoe_err_t mujoe_adc_cntrlInt_RegEOC( ADC_CTX ctx, bool enable );
extern mujoe_err_t mujoe_adc_cntrlInt_RegEOS( ADC_CTX ctx, bool enable );
extern mujoe_err_t mujoe_adc_cntrlInt_InjEOC( ADC_CTX ctx, bool enable );
extern mujoe_err_t mujoe_adc_cntrlInt_InjEOS( ADC_CTX ctx, bool enable );


// ISR callbacks
extern void mujoe_adc2_grpInjUnitaryConvComplete_CB();


#endif /* MUJOE_ADC_H_ */
