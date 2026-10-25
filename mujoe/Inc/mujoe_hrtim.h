/*
 * mujoe_hrtim.h
 *
 *  Created on: Sep 21, 2025
 *      Author: Joe Corteo
 */

#ifndef MUJOE_HRTIM_H_
#define MUJOE_HRTIM_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "hrtim.h"
#include "stm32g4xx_ll_hrtim.h"
#include "mujoe_dma.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef struct _hrtim_ctx_cfg
{

}hrtim_ctx_cfg_t;

typedef struct _hrtim_ctx
{
	hrtim_ctx_cfg_t  		cfg;
	HRTIM_HandleTypeDef		*hal_hdl;

}hrtim_ctx_t, *HRTIM_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_hrtim_init_context( HRTIM_CTX ctx,
											 HRTIM_HandleTypeDef *hal_hdl,
											 hrtim_ctx_cfg_t *pCfg );


extern mujoe_err_t mujoe_hrtim_set_tim_prescaler( HRTIM_CTX ctx, uint32_t timer, uint32_t prescaler );

extern mujoe_err_t mujoe_hrtim_TIMx_set_period( HRTIM_CTX ctx, uint32_t timer, uint32_t period );

extern uint32_t mujoe_hrtim_get_tim_freq( HRTIM_CTX ctx, uint32_t timer );

extern mujoe_err_t mujoe_hrtim_enable_timers(HRTIM_CTX ctx, uint32_t timers);
extern mujoe_err_t mujoe_hrtim_enable_all_timers(HRTIM_CTX ctx );
extern mujoe_err_t mujoe_hrtim_disable_timers(HRTIM_CTX ctx, uint32_t timers);
extern mujoe_err_t mujoe_hrtim_disable_all_timers(HRTIM_CTX ctx );
extern mujoe_err_t mujoe_hrtim_enable_output(HRTIM_CTX ctx, uint32_t output);
extern mujoe_err_t mujoe_hrtim_controlGateDriverOutputs(HRTIM_CTX ctx, bool enable);
extern mujoe_err_t mujoe_hrtim_disable_output(HRTIM_CTX ctx, uint32_t output);
extern mujoe_err_t mujoe_hrtim_setOutputSetSrc(HRTIM_CTX ctx, uint32_t output, uint32_t setSource);


extern void mujoe_hrtim_TIMx_forceUpdate( HRTIM_CTX ctx, uint32_t timers );
extern void mujoe_hrtim_TIMx_forceUpdate_single( HRTIM_CTX ctx, uint32_t timer );
extern void mujoe_hrtim_TIMx_forceUpdateAll( HRTIM_CTX ctx );

extern void mujoe_hrtim_TIMx_counter_reset( HRTIM_CTX ctx, uint32_t timers );
extern void mujoe_hrtim_TIMx_counter_reset_all( HRTIM_CTX ctx );

extern mujoe_err_t mujoe_hrtim_TIMx_setCompareX( HRTIM_CTX ctx, uint32_t timer, uint8_t compare_ch, uint32_t compare_val);

extern mujoe_err_t mujoe_hrtim_TIMx_cntrl_interrupt( HRTIM_CTX ctx, uint32_t timer, bool enable );

extern uint32_t mujoe_hrtim_getOutputLevel( HRTIM_CTX ctx, uint32_t output );

extern mujoe_err_t mujoe_hrtim_TIMx_controlCompareXInterrupt( HRTIM_CTX ctx,
		uint32_t timer,
		uint8_t compare_ch,
		bool enable );

extern mujoe_err_t mujoe_hrtim_TIMx_controlCaptureXInterrupt( HRTIM_CTX ctx,
		uint32_t timer,
		uint8_t cap_ch,
		bool enable );

extern mujoe_err_t mujoe_hrtim_captureDmaRequestControl( HRTIM_CTX ctx,
												uint32_t timer,
												uint8_t capture,
												bool enable );

extern mujoe_err_t mujoe_hrtim_compareDmaRequestControl( HRTIM_CTX ctx,
												uint32_t timer,
												uint8_t compareCh,
												bool enable );

extern mujoe_err_t mujoe_hrtim_setUpdateGating( HRTIM_CTX ctx, uint32_t timer, uint32_t updateGating );
extern void mujoe_hrtim_configBurstDMA( HRTIM_CTX ctx, uint32_t timer, uint32_t registers );

typedef struct _hrtim_dma_burst_cfg
{
	DMA_TypeDef *DMA;
	uint32_t dmaChannel;
	uint32_t dmaRequestId;
	uint32_t SrcAddress;
	uint32_t numRegisters;	// Total number of 32bit wide registers data to transfer
	bool	 circularModeEn;
	uint32_t priority;
	bool	 enableInterrupts;

}hrtim_dma_burst_cfg_t;

extern mujoe_err_t mujoe_hrtim_dma_burst_config( HRTIM_CTX ctx, hrtim_dma_burst_cfg_t *pCfg);


#endif /* MUJOE_HRTIM_H_ */
