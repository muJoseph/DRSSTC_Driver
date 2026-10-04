/*
 * mujoe_hrtim_timx_settings.h
 *
 *  Created on: Sep 25, 2025
 *      Author: Joe
 */

#ifndef MUJOE_HRTIM_TIMX_SETTINGS_H_
#define MUJOE_HRTIM_TIMX_SETTINGS_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "stm32g4xx_ll_hrtim.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////

#define HRTIM_TIMX_NUM_COMPARE_UNIT	4
#define HRTIM_TIMX_NUM_CAPTURE_UNIT 2
#define HRTIM_TIMX_NUM_OUT_CH		2


#define HRTIM_TIM_MAX_PERIOD		0x0000FFFF


#define HRTIM_NUM_TIMERS			6
////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////



typedef struct _htrim_timx_dlyprt_cfg
{
	bool 	 enabled;
	uint32_t mode;
}htrim_timx_dlyprt_cfg_t;

typedef struct _hrtim_timx_out_cfg
{
	bool	 enabled;				// If true, output is enabled
	bool	 positivePolarity;		// If true, output is HIGH when ACTIVE, LOW when INACTIVE, and vice versa.
	uint32_t Output;				// Ouput ID
	uint32_t setSrcs;				// Output SET sources
	uint32_t rstSrcs;				// Output RESET sources

}hrtim_timx_out_cfg_t;

typedef struct _hrtim_timx_cap_cfg
{
	uint32_t capTrig;
}hrtim_timx_cap_cfg_t;

typedef struct _hrtim_timx_eev_blank
{
	uint32_t event;
	uint32_t filter;
	bool	 latchEn;

}hrtim_timx_eev_blank_t;

typedef struct _hrtim_timx_cfg
{
//	NOTE: When the preload mode is enabled, accessed registers are shadow registers.
//	      Their content is transferred into the active register after an update request,
//	      either software or synchronized with an event.
	bool					preloadEnabled;				// If true, preload is enabled
	uint32_t 				pscRatio;					// Prescaler mul/div ratio
	uint32_t				timPeriod;					// Timer period
	uint32_t				updateTrig;					// Triggers that will cause the timer to update
	uint32_t 				resetTrig;					// Triggers that can reset timer
	bool 					triggerHalfModeEn;			// Triggered half-mode enabled on CMP2
	uint32_t				countingMode;				// Up/down
	uint32_t				counterMode;				// Continuous, single-shot, re-triggerable
	htrim_timx_dlyprt_cfg_t dlyPrt;					    // Delayed Protection
	//hrtim_timx_cmp_cfg_t	cmpCfg[HRTIM_TIMX_NUM_COMPARE_UNIT];	// Compare 1-4 config
	bool					cmp1EqualToMode;			// If true, Compare 1 evt gen on match. If false, evt gen when greater than
	uint32_t				cmp2Mode;					// Compare 2 Mode, supports auto delay
	bool					cmp3EqualToMode;			// If true, Compare 3 evt gen on match. If false, evt gen when greater than
	uint32_t				cmp4Mode;					// Compare 4 Mode, supports auto delay
	hrtim_timx_cap_cfg_t	capCfg[HRTIM_TIMX_NUM_CAPTURE_UNIT];	// Capture 1-2 config
	hrtim_timx_out_cfg_t	outCfg[HRTIM_TIMX_NUM_OUT_CH];			// Output Tx1 and Tx2 config
	hrtim_timx_eev_blank_t  *pEEVBlankFltrs;			// EEV Blanking/Filter table
	uint32_t				numBlankFltrs;				// Number of blanking filter(s) within table

}hrtim_timx_cfg_t;


typedef struct _hrtim_timx_cfg_tbl_entry
{
	uint32_t			timer;
	hrtim_timx_cfg_t	*pCfg;
}hrtim_timx_cfg_tbl_entry_t;

////////////////////////////////////////////////////////////////////////
// EXTERN VAR
////////////////////////////////////////////////////////////////////////

extern const hrtim_timx_cfg_t hrtim_timA_cfg;
extern const hrtim_timx_cfg_t hrtim_timB_cfg;
extern const hrtim_timx_cfg_t hrtim_timC_cfg;
extern const hrtim_timx_cfg_t hrtim_timD_cfg;
extern const hrtim_timx_cfg_t hrtim_timE_cfg;
extern const hrtim_timx_cfg_t hrtim_timF_cfg;

extern const hrtim_timx_cfg_tbl_entry_t hrtim_timx_cfg_tbl[HRTIM_NUM_TIMERS];



#endif /* #ifndef MUJOE_HRTIM_TIMX_SETTINGS_H_ */
