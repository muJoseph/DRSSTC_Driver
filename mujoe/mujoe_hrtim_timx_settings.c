/*
 * mujoe_hrtim_timx_settings.c
 *
 *  Created on: Sep 25, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_hrtim_timx_settings.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////

// NOTE: Uncomment at own risk, disabling delayed protection
// effectively disables the interrupter as a gating source (in this current
// implementation, the interruptor source being TIM5). This will cause the
// the H-bridge drivers to be driven at near 100% duty cycle causing catastrophic
// damage to the GDT gate driver ICs and/or GDTs themselves.

//#define HRTIM_TIMB_DLYPRT_DISABLE
//#define HRTIM_TIMC_DLYPRT_DISABLE

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

// HRTIM TIMA EEV Blanking Filter(s)
static const hrtim_timx_eev_blank_t timA_eev_blankFltrs[] =
{
	// EEV1 - COMP2 (CT1) Rising Edge: Blanked from reset to cmp1
	{.event = LL_HRTIM_EVENT_1, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP1, .latchEn = false},
	// EEV4 - COMP1 (ZCD CT1) Rising Edge: Blanked from reset to cmp1
	// TODO: EEV4 not used, remove
	{.event = LL_HRTIM_EVENT_4, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP1, .latchEn = false},
	// EEV6 - COMP1 (ZCD CT1, filtered) Rising Edge: Blanked from reset to cmp1
	{.event = LL_HRTIM_EVENT_6, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP1, .latchEn = false},
	// EEV9 - COMP4 (Delayed Protection/Burst): Blanked from reset to cmp2
	// postpone (event is latched) eev9 (burst start) to not interfere with period capture TODO remove?
	{.event = LL_HRTIM_EVENT_9, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = true},
};

// HRTIM TIMER A Config
const hrtim_timx_cfg_t hrtim_timA_cfg =
{
	.preloadEnabled 			= true,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers:
	// On timer reset/roll-over
	.updateTrig					= LL_HRTIM_UPDATETRIG_RESET,
	// Reset triggers
	// EEV6: COMP1 Output, digitally filtered (ZCD CT1 Tap)
	.resetTrig 					= LL_HRTIM_RESETTRIG_EEV_6,
	// Triggered Half mode DISABLED
	.triggerHalfModeEn			= false,
	// Delayed Protection
	// DISABLED
	.dlyPrt = { .enabled = false, .mode = 0 },
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	.capCfg =
	{
			// Capture 1 cfg
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE},
			// Capture 2 cfg
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE },
	},

	.cmp1EqualToMode = true,
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	.cmp3EqualToMode = true,
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	.outCfg =
	{
			// TA1 - disabled
			{
				.enabled = false,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TA1,
				.setSrcs = LL_HRTIM_OUTPUTSET_NONE,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_NONE,
			},
			// TA2
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TA2,
				.setSrcs = LL_HRTIM_OUTPUTSET_TIMAEV5_TIMDCMP1 | LL_HRTIM_OUTPUTSET_EEV_9,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_8,
			},
	},
	.pEEVBlankFltrs				= (hrtim_timx_eev_blank_t *)timA_eev_blankFltrs,
	.numBlankFltrs			 	= sizeof(timA_eev_blankFltrs)/sizeof(hrtim_timx_eev_blank_t),
}; // hrtim_timA_cfg

// HRTIM TIMB EEV Blanking Filter(s)
static const hrtim_timx_eev_blank_t timB_eev_blankFltrs[] =
{
	// EEV1 - COMP2 (CT1) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5).
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	// TODO: Remove this, EEV1 not used
	{.event = LL_HRTIM_EVENT_1, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV2 - TIM2 TRGO Rising Edge: Blanked from reset to cmp1
	{.event = LL_HRTIM_EVENT_2, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP1, .latchEn = false},
	// EEV4 - COMP1 (ZCD CT1) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5)
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	// TODO: Remove this, EEV4 not used
	{.event = LL_HRTIM_EVENT_4, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV6 - COMP1 (ZCD CT1, filtered) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5)
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	{.event = LL_HRTIM_EVENT_6, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV8 - COMP3 (OCD) Rising Edge: Blanking from another timing unit (TIMFLT3 src, e.g. TIM A Ch2 Output, TA2)
	// This is to avoid DMA overrun from multiple OCD events (while still allowing TIMD resets)
	{.event = LL_HRTIM_EVENT_8, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMBEEF3_TIMAOUT2, .latchEn = false},
};

// HRTIM TIMER B Config
const hrtim_timx_cfg_t hrtim_timB_cfg =
{
	.preloadEnabled 			= true,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers:
	// On timer reset/roll-over
	.updateTrig					= LL_HRTIM_UPDATETRIG_RESET,
	// Reset triggers
	// EEV2: TIM2 TRGO
	// EEV6: COMP1 Output, digitally filtered (ZCD CT1 Tap)
	.resetTrig 					= LL_HRTIM_RESETTRIG_EEV_2 | LL_HRTIM_RESETTRIG_EEV_6,
#if TIMB_TIMC_CMP2_DMA_EN==1
	// Triggered Half mode DISABLED
	.triggerHalfModeEn			= false,
#else
	// Triggered Half mode ENABLED
	.triggerHalfModeEn			= true,
#endif // #if TIMB_TIMC_CMP2_DMA_EN==1

#ifdef HRTIM_TIMB_DLYPRT_DISABLE
	// Delayed Protection
	// Disabled:
	.dlyPrt = { .enabled = false, .mode = LL_HRTIM_DLYPRT_DELAYBOTH_EEV7 },
#else
	// Delayed Protection
	// Enabled: Output 1 and output 2 delayed idle on external event 7 (COMP4 out)
	.dlyPrt = { .enabled = true, .mode = LL_HRTIM_DLYPRT_DELAYBOTH_EEV7 },
#endif // #ifdef HRTIM_TIMB_DLYPRT_DISABLE
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	.capCfg =
	{
			// Capture 1 config
			// Capture triggers:
		    // EEV2: TIM2 TRGO
			// EEV6: COMP1 Output, digitally filtered (ZCD CT1 Tap)
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_2 | LL_HRTIM_CAPTURETRIG_EEV_6 },
			// Capture 2 cfg
			// Capture trigger(s):
			// EEV8: COMP3 (OCD, rising)
			// NOTE: Captured value is not used, the Capture 2 event is simply
			// used to trigger a DMA2 request on CH2
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_8  },
	},

	.cmp1EqualToMode = true,
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	.cmp3EqualToMode = true,
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	.outCfg =
	{
			// TB1 - Half Bridge 1 High Side
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TB1,
				.setSrcs = LL_HRTIM_OUTPUTSET_TIMCMP2,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_2 | LL_HRTIM_OUTPUTRESET_EEV_6,
			},
			// TB2 - Half Bridge 1 Low Side
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TB2,
				.setSrcs = LL_HRTIM_OUTPUTSET_EEV_2 | LL_HRTIM_OUTPUTSET_EEV_6,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_TIMCMP2,
			},
	},
	.pEEVBlankFltrs				= (hrtim_timx_eev_blank_t *)timB_eev_blankFltrs,
	.numBlankFltrs			 	= sizeof(timB_eev_blankFltrs)/sizeof(hrtim_timx_eev_blank_t),
}; // hrtim_timB_cfg

// HRTIM TIMC EEV Blanking Filter(s)
static const hrtim_timx_eev_blank_t timC_eev_blankFltrs[] =
{
	// EEV1 - COMP2 (CT1) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5).
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	{.event = LL_HRTIM_EVENT_1, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV2 - TIM2 TRGO Rising Edge: Blanked from reset to cmp1
	{.event = LL_HRTIM_EVENT_2, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP1, .latchEn = false},
	// EEV4 - COMP1 (ZCD CT1) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5)
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	{.event = LL_HRTIM_EVENT_4, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV6 - COMP1 (ZCD CT1, filtered) Rising Edge: Blanked from reset to cmp2 (half-triggered mode: 1/f_resonant*0.5)
	// This is blanking for quarter period (1/f_resonant*0.25) after TIM2 TRGO
	{.event = LL_HRTIM_EVENT_6, .filter = LL_HRTIM_EEFLTR_BLANKINGCMP2, .latchEn = false},
	// EEV8 - COMP3 (OCD) Rising Edge: Blanking from another timing unit (TIMFLT7 src, e.g. TIM D Ch2 Output, TD2)
	// This is to avoid DMA overrun from multiple OCD events (while still allowing TIMD resets)
	{.event = LL_HRTIM_EVENT_8, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMCEEF7_TIMDOUT2, .latchEn = false},
};

// HRTIM TIMER C Config
const hrtim_timx_cfg_t hrtim_timC_cfg =
{
	.preloadEnabled 			= true,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers:
	// On timer reset/roll-over
	.updateTrig					= LL_HRTIM_UPDATETRIG_RESET,
	// Reset triggers
	// EEV2: TIM2 TRGO
	// EEV6: COMP1 Output, digitally filtered (ZCD CT1 Tap)
	.resetTrig 					= LL_HRTIM_RESETTRIG_EEV_2 | LL_HRTIM_RESETTRIG_EEV_6,
#if TIMB_TIMC_CMP2_DMA_EN==1
	// Triggered Half mode DISABLED
	.triggerHalfModeEn			= false,
#else
	// Triggered Half mode ENABLED
	.triggerHalfModeEn			= true,
#endif // #if TIMB_TIMC_CMP2_DMA_EN==1

#ifdef HRTIM_TIMC_DLYPRT_DISABLE
	// Delayed Protection
	// Disabled
	.dlyPrt = { .enabled = false, .mode = LL_HRTIM_DLYPRT_DELAYBOTH_EEV7 },
#else
	// Delayed Protection
	// Enabled: Output 1 and output 2 delayed idle on external event 7 (COMP4 out)
	.dlyPrt = { .enabled = true, .mode = LL_HRTIM_DLYPRT_DELAYBOTH_EEV7 },
#endif // #ifdef HRTIM_TIMC_DLYPRT_DISABLE
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	.capCfg =
	{
			// Capture 1 config
			// Capture triggers:
		    // EEV2: TIM2 TRGO
			// EEV6: COMP1 Output, digitally filtered (ZCD CT1 Tap)
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_2 | LL_HRTIM_CAPTURETRIG_EEV_6 },
			// Capture 2 cfg
			// Capture trigger(s):
			// EEV8: COMP3 (OCD, rising)
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_8  },
	},

	.cmp1EqualToMode = true,
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	.cmp3EqualToMode = true,
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	.outCfg =
	{
			// TC1 - Half Bridge 2 High Side
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TC1,
				.setSrcs = LL_HRTIM_OUTPUTSET_EEV_2 | LL_HRTIM_OUTPUTSET_EEV_6,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_TIMCMP2,
			},
			// TC2 - Half Bridge 2 Low Side
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TC2,
				.setSrcs = LL_HRTIM_OUTPUTSET_TIMCMP2,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_2 | LL_HRTIM_OUTPUTRESET_EEV_6 ,
			},
	},
	.pEEVBlankFltrs				= (hrtim_timx_eev_blank_t *)timC_eev_blankFltrs,
	.numBlankFltrs			 	= sizeof(timC_eev_blankFltrs)/sizeof(hrtim_timx_eev_blank_t),
}; // hrtim_timC_cfg


// HRTIM TIMER D Config (Pulse Skipping)
const hrtim_timx_cfg_t hrtim_timD_cfg =
{
	// Preload: DISABLED
	// TODO: Confirm that this should actually be disabled and not just a bug in reference design
	.preloadEnabled 			= false,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers: NONE
	.updateTrig					= LL_HRTIM_UPDATETRIG_NONE,
	// Reset triggers
	// EEV8: COMP3 Out (OCD rising edge)
	.resetTrig 					= LL_HRTIM_RESETTRIG_EEV_8,
	// Triggered Half mode: DISABLED
	.triggerHalfModeEn			= false,
	// Delayed Protection : DISABLED
	.dlyPrt = { .enabled = false, .mode = 0 },
	// Up counter
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	// Single shot, retriggerable
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	.capCfg =
	{
			// Capture 1 cfg
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE},
			// Capture 2 cfg
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE },
	},

	.cmp1EqualToMode = true,
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	.cmp3EqualToMode = true,
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	.outCfg =
	{
			// TD1 - DISABLED
			{
				.enabled = false,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TD1,
				.setSrcs = LL_HRTIM_OUTPUTSET_NONE,
				.rstSrcs = LL_HRTIM_OUTPUTRESET_NONE,
			},
			// TD2
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TD2,
				// Set source(s):
				// - Timer compare 1 event forces the output to its active state
				// - EEV9: COMP4 Out (Delayed protection, rising edge)
				.setSrcs = LL_HRTIM_OUTPUTSET_TIMCMP1 | LL_HRTIM_OUTPUTSET_EEV_9,
				// Reset source(s):
				// - EEV8: COMP 3 Out (OCD, rising edge)
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_8,
			},
	},
	// No blanking filters
	.pEEVBlankFltrs				= NULL,
	.numBlankFltrs			 	= 0,
}; // hrtim_timD_cfg


// HRTIM TIME EEV Blanking Filter(s)
static const hrtim_timx_eev_blank_t timE_eev_blankFltrs[] =
{
	// EEV1 - COMP2 Out (ZCD CT2)
	// Blanking from another timing unit: TIMFLTR5 (TIMF Output 2, TF2)
	// Blanking programmed as a waveform on Tx2 output. In this case, events are MASKED as long as
	// the Tx2 signal is INACTIVE (it is not necessary to have the output enabled, the signal is
	// taken prior to the output stage).
	// Resultant effect: event blanking for ignoring period capture events outside the
	// startFreq_maxPosDev_percent / startFreq_maxNegDev_percent window
	// TODO: EEV1 currently not used, remove this since it is useless
	{.event = LL_HRTIM_EVENT_1, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMEEEF5_TIMFOUT2, .latchEn = false},
	// TODO: EEV4 currently not used, remove this since it is useless
	// EEV4: COMP1 Out (ZCD CT1)
	{.event = LL_HRTIM_EVENT_4, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMEEEF5_TIMFOUT2, .latchEn = false},
	// EEV6: COMP1 OUT, filtered(ZCD CT1)
	//TODO: Add EEV6 TIME blanking back in!!!
	{.event = LL_HRTIM_EVENT_6, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMEEEF5_TIMFOUT2, .latchEn = false},

}; // timF_eev_blankFltrs

// HRTIM TIMER E Config (Frequency Tracking)
const hrtim_timx_cfg_t hrtim_timE_cfg =
{
	// Register preload: ENABLED
	.preloadEnabled 			= true,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	// Timer period register (PER): Max value (e.g. unused for timebase gen)
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers:
	// Timer is updated when TIMA updates
	.updateTrig					= LL_HRTIM_UPDATETRIG_TIMER_A,
	// Reset triggers
	// The timer counter is reset upon update event (and therefore on TIMA reset per update trigger setting)
	.resetTrig 					= LL_HRTIM_RESETTRIG_UPDATE,
	// Triggered Half mode: DISABLED
	.triggerHalfModeEn			= false,
	// Delayed Protection: DISABLED
	.dlyPrt = { .enabled = false, .mode = 0 },
	// Up counter
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	// Single-shot, re-triggerable
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	// Capture configuration
	.capCfg =
	{
			// Capture 1 config
			// Capture triggers:
			// EEV6: COMP1 Out, filtered (ZCD CT1, rising edge)
			// This captures the current resonant period at each zero-crossing of the current
			// waveform
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_6 },
			// Capture 2 cfg
			// Capture trigger(s): NONE
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE  },
	},

	// Compare 1 Mode: Equal to
	.cmp1EqualToMode = true,
	// Compare 2 Mode: regular
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	// Compare 3 Mode: Equal to
	.cmp3EqualToMode = true,
	// Compare 4 Mode: regular
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	// Output configuration
	.outCfg =
	{
			// TE1 - DISABLED
			{
				.enabled = false,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TE1,
				// TE1 Set: None
				.setSrcs = LL_HRTIM_OUTPUTSET_NONE,
				// TE1 Reset: None
				.rstSrcs = LL_HRTIM_OUTPUTRESET_NONE,
			},
			// TE2
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TE2,
				// TE2 Set:
				// EEV7: COMP4 Out (Delayed protection, low level)
				.setSrcs = LL_HRTIM_OUTPUTSET_EEV_7,
				// TE2 Reset:
				// EEV9: COMP4 Out (Delayed protection, rising edge)
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_9,
			},
	},
	.pEEVBlankFltrs				= (hrtim_timx_eev_blank_t *)timE_eev_blankFltrs,
	.numBlankFltrs			 	= sizeof(timE_eev_blankFltrs)/sizeof(hrtim_timx_eev_blank_t),
}; // hrtim_timE_cfg


// HRTIM TIMF EEV Blanking Filter(s)
static const hrtim_timx_eev_blank_t timF_eev_blankFltrs[] =
{
	// EEV2 - TIM2 TRGO Rising Edge: Blanking from another timing unit: TIMFLTR8 (Timer E Output 2,TE2)
	// Blanking programmed as a waveform on Tx2 output. In this case, events are MASKED as long as
	// the Tx2 signal is INACTIVE (it is not necessary to have the output enabled, the signal is
	// taken prior to the output stage).
	// Resultant effect:  TIM2 TRGO only allowed to reset TIMF and cause capture into cmp2
	// during burst for timeout detection
	{.event = LL_HRTIM_EVENT_2, .filter = LL_HRTIM_EEFLTR_BLANKING_TIMFEEF8_TIMEOUT2, .latchEn = false},

}; // timF_eev_blankFltrs

// HRTIM TIMER F Config
const hrtim_timx_cfg_t hrtim_timF_cfg =
{
	// Register preload: ENABLED
	.preloadEnabled 			= true,
	// f_tim = f_hrtim/1 = 170e6/1 = 170 MHz
	.pscRatio 					= LL_HRTIM_PRESCALERRATIO_DIV1,
	// Timer period register (PER): Max value (e.g. unused for timebase gen)
	.timPeriod					= HRTIM_TIM_MAX_PERIOD,
	// Update triggers:
	// Timer is updated when TIMA updates
	.updateTrig					= LL_HRTIM_UPDATETRIG_TIMER_A,
	// Reset triggers
	// The timer counter is reset upon update event (and therefore on TIMA reset per update trigger setting)
	// EEV2: TIM2 TRGO, blanked/filtered by TIM E Output 2 waveform (TE2)
	.resetTrig 					= LL_HRTIM_RESETTRIG_UPDATE | LL_HRTIM_RESETTRIG_EEV_2,
	// Triggered Half mode: DISABLED
	.triggerHalfModeEn			= false,
	// Delayed Protection: DISABLED
	.dlyPrt = { .enabled = false, .mode = 0 },
	// Up counter
	.countingMode 				= LL_HRTIM_COUNTING_MODE_UP,
	// Single-shot, retriggerable
	.counterMode				= LL_HRTIM_MODE_RETRIGGERABLE,
	.capCfg =
	{
			// Capture 1 config
			// Capture triggers: NONE
			{ .capTrig = LL_HRTIM_CAPTURETRIG_NONE },
			// Capture 2 cfg
			// Capture trigger(s):
			// EEV2: TIM2 TRGO
			{ .capTrig = LL_HRTIM_CAPTURETRIG_EEV_2  },
	},

	// Compare 1 Mode: Equal to
	.cmp1EqualToMode = true,
	// Compare 2 Mode: regular
	.cmp2Mode = LL_HRTIM_COMPAREMODE_REGULAR,
	// Compare 3 Mode: Equal to
	.cmp3EqualToMode = true,
	// Compare 4 Mode: regular
	.cmp4Mode = LL_HRTIM_COMPAREMODE_REGULAR,

	// Output configuration
	.outCfg =
	{
			// TF1
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TF1,
				// TF1 Set:
				// EEV6 (ZCD COMP1) : TODO: This is considered debug, remove
				.setSrcs = LL_HRTIM_OUTPUTSET_EEV_6,
				// TF1 Reset:
				// EEV6 (ZCD COMP1) : TODO: This is considered debug, remove
				.rstSrcs = LL_HRTIM_OUTPUTRESET_EEV_6,
			},
			// TF2 - Used for Blanking Window for filtering EEV6 (ZCD) for TIME frequency tracking
			{
				.enabled = true,
				.positivePolarity = true,
				.Output  = LL_HRTIM_OUTPUT_TF2,
				// TF2 Set:
				// On Timer EVT 8: TIME cmp2 match
				.setSrcs = LL_HRTIM_OUTPUTSET_TIMFEV8_TIMECMP2,
				// TF2 Reset:
				// - Timer register update event forces the output to its inactive state
				// - On Timer EVT 9: TIME cmp3 match
				// - EEV7: Delayed Protection (COMP4 Out)
				.rstSrcs = LL_HRTIM_OUTPUTRESET_UPDATE | LL_HRTIM_OUTPUTRESET_TIMFEV9_TIMECMP3 | LL_HRTIM_OUTPUTRESET_EEV_7,
			},
	},
	.pEEVBlankFltrs				= (hrtim_timx_eev_blank_t *)timF_eev_blankFltrs,
	.numBlankFltrs			 	= sizeof(timF_eev_blankFltrs)/sizeof(hrtim_timx_eev_blank_t),
}; // hrtim_timF_cfg


// HRTIM Configuration table
const hrtim_timx_cfg_tbl_entry_t  hrtim_timx_cfg_tbl[HRTIM_NUM_TIMERS] =
{
	{ .timer = LL_HRTIM_TIMER_A, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timA_cfg},
	{ .timer = LL_HRTIM_TIMER_B, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timB_cfg},
	{ .timer = LL_HRTIM_TIMER_C, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timC_cfg},
	{ .timer = LL_HRTIM_TIMER_D, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timD_cfg},
	{ .timer = LL_HRTIM_TIMER_E, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timE_cfg},
	{ .timer = LL_HRTIM_TIMER_F, .pCfg = (hrtim_timx_cfg_t*)&hrtim_timF_cfg},

};// hrtim_timx_cfg_tbl



