/*
 * mujoe_bsp.h
 *
 *      Author: mujoe
 */

#ifndef __MUJOE_BSP_H__
#define __MUJOE_BSP_H__

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "stm32g474xx.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern void mujoe_bsp_ctrl_LED( bool on );
extern bool mujoe_bsp_is_LED_ON( void );

extern void mujoe_bsp_ctrl_GDT_EN( bool enable );
extern void mujoe_bsp_ctrl_BUZZER( bool on );
extern bool mujoe_bsp_is_BUZZER_ON( void );

extern bool mujoe_bsp_is_HV_ON( void );

extern bool mujoe_bsp_ctrl_ICL_BYPASS( bool enable );
// Turn on/off high voltage SSR
extern bool mujoe_bsp_ctrl_HV_ON( bool on );
extern bool mujoe_bsp_ctrl_HV_BLEED( bool enable );

extern bool mujoe_bsp_powerOnHighVoltage( void );
extern bool mujoe_bsp_powerOffHighVoltage( void );

#endif // #ifndef __MUJOE_BSP_H__
