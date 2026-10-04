/*
 * mujoe_resdrv.h
 *
 *  Created on: Sep 15, 2025
 *      Author: Joe
 */

#ifndef MUJOE_RESDRV_H_
#define MUJOE_RESDRV_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_comp.h"
#include "mujoe_dac.h"
#include "mujoe_dma.h"
#include "mujoe_gptim.h"
#include "mujoe_hrtim.h"
#include "mujoe_adc.h"

#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	RESDRV_PRESET_ID_DEFAULT = (uint8_t)0,
	RESDRV_PRESET_ID_DEMO,
	RESDRV_PRESET_ID_DEMO_NO_LEAD,
	RESDRV_PRESET_ID_CURR_FB_DBG, // Current Feedback debug
	RESDRV_PRESET_ID_CURR_FB_DBG_FAB_CT,
	RESDRV_PRESET_ID_CURR_FB_DBG_FAB_CT_NEW,
}presetId_t;


typedef enum
{
	DAC1_ID = (uint8_t)0,
	DAC3_ID,
	NUM_DACS,
}dac_id_t;

typedef enum
{
	COMP1_ID = (uint8_t)0,
	COMP2_ID,
	COMP3_ID,
	COMP4_ID,
	NUM_COMPS,
}comp_id_t;

typedef enum
{
	TIM2_ID = (uint8_t)0,
	TIM5_ID,
	NUM_GPTIMERS,
}gp_timer_id_t;


typedef struct _resdrv_dbg_cfg
{
	bool	enable_TF2OutputPin;

}resdrv_dbg_cfg_t;

// Context configuration
typedef struct _resdrv_cfg
{
	float   CTRatio_currFb;				// Primary feedback current transformer ratio
	float 	CTBurdenResistor_currFb;	// Primary feedback current transformer output burden resistor (Ohms)
	float	OCDLimit_A;					// Max primary current (Amps)
	bool	enableDMACallbacks;			// If true, DMA callbacks (and thus interrupts) are enabled
	float	startFreq_Hz;				// Starting frequency of resonant circuit (Hz)
	float	startFreq_maxPosDev_percent; // Maximum deviation above startFreq_Hz for frequency tracking (%)
	float	startFreq_maxNegDev_percent; // Maximum deviation below startFreq_Hz for frequency tracking (%)
	uint32_t	leadTime_ns;				// Lead time (nanosec)
	float	zcdFeedback_timeout_us;		// Timeout for feedback lock (usec)
	bool	enablePulseSkipping;
	bool	enableBurstReload;
	int		numBurstCycles;				// If enableBurstReload == true, numBurstCycles specifies number of
										// PRF cycle(s) to execute. 0 for infinite cycles.
	resdrv_dbg_cfg_t	dbg_cfg;
	void				*hostArg;
	// If non-zero, 'resPeriodCapBuffLen' instances of TIME.CPT1ER captures
	// will be saved via DMA transfer
	int		resPeriodCapBuffLen;
}resdrv_cfg_t;

typedef enum
{
	RESDRV_SYS_STATE_INIT = (uint8_t)0,
	RESDRV_SYS_STATE_IDLE_DISARMED,
	RESDRV_SYS_STATE_IDLE_ARMED,
	RESDRV_SYS_STATE_RUN,
	RESDRV_SYS_STATE_FAULT,

}resdrv_sys_state_t;

typedef uint32_t resdrv_fault_flag_t;

#define RESDRV_FAULT_FLAG_OCD						(resdrv_fault_flag_t)BV(0)
#define RESDRV_FAULT_FLAG_UVLO						(resdrv_fault_flag_t)BV(1)
#define RESDRV_FAULT_FLAG_FB_TIMEOUT				(resdrv_fault_flag_t)BV(2)

// Run-time variables
typedef struct _resdrv_rt
{
	float					UVLimit;		// Min voltage for gate driver supply voltage
	float					Ip_maxResolve;	// Max resolvable primary current (A)
	float					V_OCDLimit_ref;	// Reference voltage provided to OCD comparator to set OCDLimit
	resdrv_sys_state_t		sys_state;
	resdrv_fault_flag_t		fault_flag;
	uint32_t				start_period; 		// Starting period of resonant circuit, to be loaded directly into registers (unitless)
	uint32_t				prf_ontime_us;	// PRF On time (usec)
	uint32_t				prf_period_us;	// PRF period (usec)
	uint32_t				cs_adc_sample;	// Uint32 to accommodate  all ADC bit depths, including over-sampling
	int						prfCycleCnt;	// Used when number of PRF burst cycles is finite
}resdrv_rt_t;


typedef void (*resdrv_faultChange_Fp_t)(void* hostArg, resdrv_fault_flag_t fault );
typedef void (*resdrv_numPrfCyclesComplete_Fp_t)(void* hostArg );

typedef struct _resdrv_cbs
{
	resdrv_faultChange_Fp_t						faultChangeCB;
	resdrv_numPrfCyclesComplete_Fp_t			numPrfCyclesCompleteCB;

}resdrv_cbs_t;

typedef struct _resdrv_ctx
{
	resdrv_cfg_t  		cfg;
	resdrv_cbs_t		*pCbs;
	hrtim_ctx_t			hrtim;
	dma_ctx_t			dma1;
	dma_ctx_t			dma2;
	adc_ctx_t			adc2;
	dac_ctx_t			dacs[NUM_DACS];
	comp_ctx_t			comps[NUM_COMPS];
	gptim_ctx_t			gptims[NUM_GPTIMERS];
	resdrv_rt_t			rt;
	// Buffer for storing TIME CPT1ER captures
	uint32_t			*pResPeriodCapBuff;

}resdrv_ctx_t, *RESDRV_CTX;

////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////
#define IS_OCD_FAULT(F)				(((F)&RESDRV_FAULT_FLAG_OCD)==RESDRV_FAULT_FLAG_OCD)
#define IS_UVLO_FAULT(F)			(((F)&RESDRV_FAULT_FLAG_UVLO)==RESDRV_FAULT_FLAG_UVLO)
#define IS_FB_TIMEOUT_FAULT(F)		(((F)&RESDRV_FAULT_FLAG_FB_TIMEOUT)==RESDRV_FAULT_FLAG_FB_TIMEOUT)

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern void mujoe_resdrv_init_configStruct( presetId_t id, resdrv_cfg_t *pCfg );

extern mujoe_err_t mujoe_resdrv_init_context( RESDRV_CTX ctx,
											HRTIM_HandleTypeDef *hrtim_hal_hdl,
											resdrv_cfg_t *pCfg );

extern void mujoe_resdrv_registerCallbacks( RESDRV_CTX ctx, resdrv_cbs_t *pCbs);
extern mujoe_err_t mujoe_resdrv_init_hardware( RESDRV_CTX ctx );

extern mujoe_err_t mujoe_resdrv_set_OCD_limit( RESDRV_CTX ctx, float ocd_limit_a );
extern mujoe_err_t mujoe_resdrv_set_OCD_comp_input_hysterisis( RESDRV_CTX ctx, mujoe_comp_in_hysteresis_t hyst );

extern mujoe_err_t mujoe_resdrv_arm( RESDRV_CTX ctx );
extern mujoe_err_t mujoe_resdrv_disarm( RESDRV_CTX ctx );

typedef struct _mujoe_resdrv_start_burst_arg
{
    uint32_t 	startFreq_Hz;
	uint8_t		startFreq_maxNegDev_percnt;
	uint8_t		startFreq_maxPosDev_percnt;
	uint32_t 	ontime_us;
	uint32_t 	period_us;
	uint32_t 	numBurstCycles;

}mujoe_resdrv_start_burst_arg_t;
extern mujoe_err_t mujoe_resdrv_start_burst( RESDRV_CTX ctx, mujoe_resdrv_start_burst_arg_t *pArg );

extern mujoe_err_t mujoe_resdrv_stop_burst( RESDRV_CTX ctx );
extern mujoe_err_t mujoe_resdrv_set_lead_time(RESDRV_CTX ctx, uint32_t leadTime_ns);
extern mujoe_err_t mujoe_resdrv_handleFeedbackTimeout( RESDRV_CTX ctx );
extern mujoe_err_t mujoe_resdrv_handleNumPrfCyclesComplete( RESDRV_CTX ctx );

extern resdrv_fault_flag_t mujoe_resdrv_getFaults( RESDRV_CTX ctx );
extern mujoe_err_t mujoe_resdrv_ackFaults( RESDRV_CTX ctx );
extern mujoe_err_t mujoe_resdrv_restoreHbridgeOutputRouting(RESDRV_CTX ctx);
extern uint32_t mujoe_resdrv_getMeasuredResonantFrequency( RESDRV_CTX ctx );

extern uint16_t mujoe_resdrv_getResPeriodCapAvg( RESDRV_CTX ctx );
extern void mujoe_resdrv_getResPeriodCapBuffer( RESDRV_CTX ctx, uint32_t **ppBuf, int *pBufLen );


#endif /* MUJOE_RESDRV_H_ */
