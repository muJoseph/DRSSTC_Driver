/*
 * mujoe_hrtim_debug.c
 *
 *  Created on: Sep 24, 2025
 *      Author: Joe
 * 
 *  Usage Examples:
 *  - Dump specific timer: mujoe_hrtim_timx_dbg_dump_cfg(HRTIM1, 0x02); // TIMER_B
 *  - Dump all timers: mujoe_hrtim_dbg_dump_all_timers(HRTIM1);
 *  - Timer indices: 0x00=MASTER, 0x01=TIMER_A, 0x02=TIMER_B, 0x03=TIMER_C, 
 *                   0x04=TIMER_D, 0x05=TIMER_E, 0x06=TIMER_F
 *  
 *  Output Format:
 *  - @string parameters: Printed as human-readable strings (e.g., "DIV1 (1x)", "CONTINUOUS")
 *  - @decimal parameters: Printed as unsigned integers (e.g., 65535)
 *  
 *  Note: Switch cases now match actual enum values from STM32 HAL:
 *  - Prescaler: 0x00=MUL32, 0x01=MUL16, 0x02=MUL8, 0x03=MUL4, 0x04=MUL2, 0x05=DIV1, 0x06=DIV2, 0x07=DIV4
 *  - Counter Mode: 0x00=SINGLESHOT, 0x08=CONTINUOUS, 0x10=RETRIGGERABLE
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_hrtim_debug.h"
#include "mujoe_hrtim.h"
#include "stm32g474xx.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>


//typedef unsigned int uint32_t;

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

static const char* psc_enum_to_str( unsigned int psc )
{
	switch( psc )
	{
		case 0x00000000U: // LL_HRTIM_PRESCALERRATIO_MUL32
			return "MUL32 (32x)";
		case 0x00000001U: // LL_HRTIM_PRESCALERRATIO_MUL16
			return "MUL16 (16x)";
		case 0x00000002U: // LL_HRTIM_PRESCALERRATIO_MUL8
			return "MUL8 (8x)";
		case 0x00000003U: // LL_HRTIM_PRESCALERRATIO_MUL4
			return "MUL4 (4x)";
		case 0x00000004U: // LL_HRTIM_PRESCALERRATIO_MUL2
			return "MUL2 (2x)";
		case 0x00000005U: // LL_HRTIM_PRESCALERRATIO_DIV1
			return "DIV1 (1x)";
		case 0x00000006U: // LL_HRTIM_PRESCALERRATIO_DIV2
			return "DIV2 (0.5x)";
		case 0x00000007U: // LL_HRTIM_PRESCALERRATIO_DIV4
			return "DIV4 (0.25x)";
		default:
			return "UNKNOWN";
	}
}// psc_enum_to_str

static const char* timer_name_to_str( unsigned int timer )
{
	switch( timer )
	{
		case 0x00: // HRTIM_TIMERINDEX_MASTER
			return "MASTER";
		case 0x01: // HRTIM_TIMERINDEX_TIMER_A
			return "TIMER_A";
		case 0x02: // HRTIM_TIMERINDEX_TIMER_B
			return "TIMER_B";
		case 0x03: // HRTIM_TIMERINDEX_TIMER_C
			return "TIMER_C";
		case 0x04: // HRTIM_TIMERINDEX_TIMER_D
			return "TIMER_D";
		case 0x05: // HRTIM_TIMERINDEX_TIMER_E
			return "TIMER_E";
		case 0x06: // HRTIM_TIMERINDEX_TIMER_F
			return "TIMER_F";
		default:
			return "UNKNOWN";
	}
}// timer_name_to_str



static const char* counter_mode_to_str( unsigned int mode )
{
	switch( mode )
	{
		case 0x00000008U: // LL_HRTIM_MODE_CONTINUOUS
			return "CONTINUOUS";
		case 0x00000000U: // LL_HRTIM_MODE_SINGLESHOT
			return "SINGLESHOT";
		case 0x00000010U: // LL_HRTIM_MODE_RETRIGGERABLE
			return "RETRIGGERABLE";
		default:
			return "UNKNOWN";
	}
}// counter_mode_to_str

static const char* counting_mode_to_str( unsigned int mode )
{
	switch( mode )
	{
		case 0x00000000U: // LL_HRTIM_COUNTING_MODE_UP
			return "UP";
		case 0x00000001U: // LL_HRTIM_COUNTING_MODE_UP_DOWN
			return "UP_DOWN";
		default:
			return "UNKNOWN";
	}
}// counting_mode_to_str



typedef struct _hrtim_timx_dump_params
{
	unsigned int psc_ratio;		// @string
	unsigned int arr;			// @decimal
	unsigned int counter_mode;	// @string
	unsigned int counting_mode; // @string

}hrtim_timx_dump_params_t;

void mujoe_hrtim_timx_dbg_dump_cfg( HRTIM_TypeDef *HRTIM, unsigned int timer )
{
	printf("\n=== HRTIM %s Configuration Dump ===\n", timer_name_to_str(timer));
	
	// Read parameters from HRTIM registers
	hrtim_timx_dump_params_t params;
	params.psc_ratio = LL_HRTIM_TIM_GetPrescaler(HRTIM, timer);
	params.arr = LL_HRTIM_TIM_GetPeriod(HRTIM, timer);
	params.counter_mode = LL_HRTIM_TIM_GetCounterMode(HRTIM, timer);
	params.counting_mode = LL_HRTIM_TIM_GetCountingMode(HRTIM, timer);
	// Print parameters based on format specified in structure comments
	printf("  Prescaler Ratio: %s\n", psc_enum_to_str(params.psc_ratio));  // @string
	printf("  ARR (Period): %u\n", params.arr);                            // @decimal
	printf("  Counter Mode: %s\n", counter_mode_to_str(params.counter_mode)); // @string
	printf("  Counting Mode: %s\n", counting_mode_to_str(params.counting_mode)); // @string

	printf("\n=== HRTIM %s Configuration Dump END===\n", timer_name_to_str(timer));
}// mujoe_hrtim_timx_dbg_dump_cfg

void mujoe_hrtim_dbg_dump_all_timers( HRTIM_TypeDef *HRTIM )
{
	printf("\n========================================\n");
	printf("HRTIM ALL TIMERS CONFIGURATION DUMP\n");
	printf("========================================\n");
	
	// Dump all timers
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x00); // MASTER
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x01); // TIMER_A
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x02); // TIMER_B
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x03); // TIMER_C
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x04); // TIMER_D
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x05); // TIMER_E
	mujoe_hrtim_timx_dbg_dump_cfg(HRTIM, 0x06); // TIMER_F
	
	printf("========================================\n");
	printf("END HRTIM ALL TIMERS CONFIGURATION DUMP\n");
	printf("========================================\n\n");

}// mujoe_hrtim_dbg_dump_all_timers

void mujoe_hrtim_dbg_print_test_msg( const char* msg)
{
	printf("This is a test message: %s\n",msg);
}// mujoe_hrtim_dbg_print_test_msg
