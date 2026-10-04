/*
 * mujoe_timx_settings.c
 *
 *  Created on: Sep 25, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_timx_settings.h"
//#include <string.h>


////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

// TIM2 (Master Time-keeper)
const timx_cfg_t tim2_cfg =
{
		.PSC 				= 0,
		.ARR 				= 0,
		.counterMode 		= LL_TIM_COUNTERMODE_DOWN,
		.slaveMode			= LL_TIM_SLAVEMODE_RESET,
		.chCfg =
		{
			// Ch1 : TIM2 TRGO generation on CMP1 match
			{.enabled = true},
			// Ch2
			{.enabled = false},
			// Ch3
			{.enabled = false},
			// Ch4
			{.enabled = false},
		},
};// tim2_cfg


