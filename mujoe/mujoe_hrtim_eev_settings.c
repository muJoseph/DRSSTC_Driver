/*
 * mujoe_hrtim_eev_settings.c
 *
 *  Created on: Sep 25, 2025
 *      Author: Joe
 */

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_hrtim_eev_settings.h"

////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////

#define EEV_PRESET_DEFAULT				1
#define EEV_PRESET_FAST_NO_FILTER		2		// DOES NOT WORK
#define EEV_PRESET_FAST_W_FILTER		3		// DOES NOT WORK


#define EEV_PRESET						EEV_PRESET_DEFAULT

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////


#if EEV_PRESET==EEV_PRESET_DEFAULT
// EEV Event Mapping and Configuration
// NOTE: EEV1-5:  Fast mode available
//		 EEV6-10: Digital filter available
const eev_event_config_t eev_event_cfgs[HRTIM_NUM_EEV] =
{
	// EEV1 : COMP2 Output (ZCD CT2 tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV1SRC_COMP2_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= false,
	},
	// EEV2 : TIM2 TRGO
	{
	  .eventSrc 			= LL_HRTIM_EEV2SRC_TIM2_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= false,
	},

	// EEV3 : TIM3 TRGO
	// TODO: EEV3 unused by HRTIM timers, remove
	{
	  .eventSrc 			= LL_HRTIM_EEV3SRC_TIM3_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= false,
	},

	// EEV4 : COMP1 Output (ZCD CT1 Tap)
	// TODO: EEV4 unused by HRTIM timers, remove
	{
	  .eventSrc 			= LL_HRTIM_EEV4SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= false,
	},

	// EEV5 : COMP3 Output (OCD, falling edge = OCD fault ASSERTED )
	// TODO: EEV5 unused by HRTIM timers, remove
	{
	  .eventSrc 			= LL_HRTIM_EEV5SRC_COMP3_OUT,
	  .polarityHigh 		= true,	// Don't care when sensitivity is edge-detection
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE,
	  .fastModeEnable 		= false,
	},

	// EEV6 : COMP1 Output, digitally filtered (ZCD CT1 Tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV6SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_2,	// fSAMPLING= fHRTIM, N=4 (Ref Manual, pg.1107)
	},

	// EEV7 : COMP4 Output, (Delayed protection, low level)
	{
	  .eventSrc 			= LL_HRTIM_EEV7SRC_COMP4_OUT,
	  .polarityHigh 		= false,	// Low-level activation
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_LEVEL,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV8 : COMP3 Output, ( OCD, rising edge = OCD fault DE-ASSERTED )
	{
	  .eventSrc 			= LL_HRTIM_EEV8SRC_COMP3_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV9 : COMP4 Output, ( Delayed protection, rising edge)
	{
	  .eventSrc 			= LL_HRTIM_EEV9SRC_COMP4_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV10 : PC5/PC6 (NOTE: Not currently used for anything)
	// TODO: EEV10 unused by HRTIM timers, remove
	{
	  .eventSrc 			= LL_HRTIM_EEV10SRC_GPIO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},
};

#elif EEV_PRESET==EEV_PRESET_FAST_NO_FILTER
// EEV Event Mapping and Configuration
// NOTE: EEV1-5:  Fast mode available
//		 EEV6-10: Digital filter available
const eev_event_config_t eev_event_cfgs[HRTIM_NUM_EEV] =
{
	// EEV1 : COMP2 Output (ZCD CT2 tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV1SRC_COMP2_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},
	// EEV2 : TIM2 TRGO
	{
	  .eventSrc 			= LL_HRTIM_EEV2SRC_TIM2_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV3 : TIM3 TRGO (NOTE: Not used for anything at the moment)
	{
	  .eventSrc 			= LL_HRTIM_EEV3SRC_TIM3_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV4 : COMP1 Output (ZCD CT1 Tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV4SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV5 : COMP3 Output (OCD, falling edge = OCD fault ASSERTED )
	{
	  .eventSrc 			= LL_HRTIM_EEV5SRC_COMP3_OUT,
	  .polarityHigh 		= true,	// Don't care when sensitivity is edge-detection
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV6 : COMP1 Output, digitally filtered (ZCD CT1 Tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV6SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},

	// EEV7 : COMP4 Output, (Delayed protection, low level)
	{
	  .eventSrc 			= LL_HRTIM_EEV7SRC_COMP4_OUT,
	  .polarityHigh 		= false,	// Low-level activation
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_LEVEL,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},

	// EEV8 : COMP3 Output, ( OCD, rising edge = OCD fault DE-ASSERTED )
	{
	  .eventSrc 			= LL_HRTIM_EEV8SRC_COMP3_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},

	// EEV9 : COMP4 Output, ( Delayed protection, rising edge)
	{
	  .eventSrc 			= LL_HRTIM_EEV9SRC_COMP4_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},

	// EEV10 : PC5/PC6 (NOTE: Not currently used for anything)
	{
	  .eventSrc 			= LL_HRTIM_EEV10SRC_GPIO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},
};

#elif EEV_PRESET==EEV_PRESET_FAST_W_FILTER

// EEV Event Mapping and Configuration
// NOTE: EEV1-5:  Fast mode available
//		 EEV6-10: Digital filter available
const eev_event_config_t eev_event_cfgs[HRTIM_NUM_EEV] =
{
	// EEV1 : COMP2 Output (ZCD CT2 tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV1SRC_COMP2_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},
	// EEV2 : TIM2 TRGO
	{
	  .eventSrc 			= LL_HRTIM_EEV2SRC_TIM2_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV3 : TIM3 TRGO (NOTE: Not used for anything at the moment)
	{
	  .eventSrc 			= LL_HRTIM_EEV3SRC_TIM3_TRGO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV4 : COMP1 Output (ZCD CT1 Tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV4SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV5 : COMP3 Output (OCD, falling edge = OCD fault ASSERTED )
	{
	  .eventSrc 			= LL_HRTIM_EEV5SRC_COMP3_OUT,
	  .polarityHigh 		= true,	// Don't care when sensitivity is edge-detection
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_FALLINGEDGE,
	  .fastModeEnable 		= true,
	},

	// EEV6 : COMP1 Output, digitally filtered (ZCD CT1 Tap)
	{
	  .eventSrc 			= LL_HRTIM_EEV6SRC_COMP1_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_2,	// fSAMPLING= fHRTIM, N=4 (Ref Manual, pg.1107)
	},

	// EEV7 : COMP4 Output, (Delayed protection, low level)
	{
	  .eventSrc 			= LL_HRTIM_EEV7SRC_COMP4_OUT,
	  .polarityHigh 		= false,	// Low-level activation
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_LEVEL,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV8 : COMP3 Output, ( OCD, rising edge = OCD fault DE-ASSERTED )
	{
	  .eventSrc 			= LL_HRTIM_EEV8SRC_COMP3_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV9 : COMP4 Output, ( Delayed protection, rising edge)
	{
	  .eventSrc 			= LL_HRTIM_EEV9SRC_COMP4_OUT,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_3,	// fSAMPLING= fHRTIM, N=8 (Ref Manual, pg.1107)
	},

	// EEV10 : PC5/PC6 (NOTE: Not currently used for anything)
	{
	  .eventSrc 			= LL_HRTIM_EEV10SRC_GPIO,
	  .polarityHigh 		= true,
	  .sensitivity 			= LL_HRTIM_EE_SENSITIVITY_RISINGEDGE,
	  .filter 				= LL_HRTIM_EE_FILTER_NONE,
	},
};

#endif // #if EEV_PRESET==EEV_PRESET_DEFAULT
