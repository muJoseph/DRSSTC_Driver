/*
 * mujoe_hrtim_debug.h
 *
 *  Created on: Sep 24, 2025
 *      Author: Joe
 */

#ifndef HRTIM_DEBUG_H_
#define HRTIM_DEBUG_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

//#include "hrtim.h"
#include "stm32g4xx_ll_hrtim.h"
//#include <string.h>
//#include <stdio.h>

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

extern void mujoe_hrtim_timx_dbg_dump_cfg( HRTIM_TypeDef *HRTIM, unsigned int timer );
extern void mujoe_hrtim_dbg_dump_all_timers( HRTIM_TypeDef *HRTIM );

extern void mujoe_hrtim_dbg_print_test_msg( const char* msg);	// DEBUG

#endif /* HRTIM_DEBUG_H_ */
