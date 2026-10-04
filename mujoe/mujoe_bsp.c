/*
 * mujoe_bsp.c
 *
 *  Created on: Jan 18, 2026
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bsp.h"
#include "main.h"

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////


void mujoe_bsp_ctrl_LED( bool on )
{
	// Active-high
	if( on )
	{
		LL_GPIO_SetOutputPin(LED_GPIO_Port, LED_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(LED_GPIO_Port, LED_Pin);
	}

}// mujoe_bsp_ctrl_LED

bool mujoe_bsp_is_LED_ON( void )
{
	// Active high
	return LL_GPIO_IsOutputPinSet(LED_GPIO_Port, LED_Pin);

}// mujoe_bsp_is_LED_ON

void mujoe_bsp_ctrl_GDT_EN( bool enable )
{
	// Active-low
	if( enable )
	{
		LL_GPIO_ResetOutputPin(GDT_EN_GPIO_Port, GDT_EN_Pin);
	}
	else
	{
		LL_GPIO_SetOutputPin(GDT_EN_GPIO_Port, GDT_EN_Pin);
	}

}// mujoe_bsp_ctrl_LED


void mujoe_bsp_ctrl_BUZZER( bool on )
{
	if( on )
	{
		LL_GPIO_SetOutputPin(BUZZER_GPIO_Port, BUZZER_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(BUZZER_GPIO_Port, BUZZER_Pin);
	}

}// mujoe_bsp_ctrl_BUZZER


bool mujoe_bsp_is_BUZZER_ON( void )
{
	return LL_GPIO_IsOutputPinSet(BUZZER_GPIO_Port, BUZZER_Pin);

}// mujoe_bsp_ctrl_BUZZER

bool mujoe_bsp_is_ICL_BYPASS_ENABLED( void )
{
	// If high, ICL bypass is enabled
	return LL_GPIO_IsOutputPinSet(HVBUS_ICL_BYPASS_GPIO_Port, HVBUS_ICL_BYPASS_Pin);

}// mujoe_bsp_ctrl_BUZZER

// Control ICL bypass circuit
bool mujoe_bsp_ctrl_ICL_BYPASS( bool enable )
{
	// Active high
	if( enable )
	{
		LL_GPIO_SetOutputPin(HVBUS_ICL_BYPASS_GPIO_Port, HVBUS_ICL_BYPASS_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(HVBUS_ICL_BYPASS_GPIO_Port, HVBUS_ICL_BYPASS_Pin);
	}

	return ( mujoe_bsp_is_ICL_BYPASS_ENABLED() == enable );

}// mujoe_bsp_ctrl_ICL_BYPASS


bool mujoe_bsp_is_HV_ON( void )
{
	return LL_GPIO_IsOutputPinSet(HV_ON_GPIO_Port, HV_ON_Pin);

}// mujoe_bsp_is_HV_ON

// Turn on/off high voltage SSR
bool mujoe_bsp_ctrl_HV_ON( bool on )
{
	// Active high
	if( on )
	{
		LL_GPIO_SetOutputPin(HV_ON_GPIO_Port, HV_ON_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(HV_ON_GPIO_Port, HV_ON_Pin);
	}

	return ( mujoe_bsp_is_HV_ON() == on );

}// mujoe_bsp_ctrl_HV_ON


bool mujoe_bsp_powerOnHighVoltage( void )
{
	// Ensure that ICL bypass is DISABLED before enabling hv bus
	if( !mujoe_bsp_is_ICL_BYPASS_ENABLED() )
	{
		// Disable HV bleed resistor
		mujoe_bsp_ctrl_HV_BLEED( false );
		// Power on high voltage bus
		mujoe_bsp_ctrl_HV_ON( true );
		return true;
	}
	else
	{
		return false;
	}
}// mujoe_bsp_powerOnHighVoltage

bool mujoe_bsp_powerOffHighVoltage( void )
{
	// Disable HV bus
	if( mujoe_bsp_ctrl_HV_ON( false ) )
	{
		// Enable bleed resistor
		return mujoe_bsp_ctrl_HV_BLEED( true );
	}
	else
	{
		return false;
	}

}// mujoe_bsp_powerOnHighVoltage



bool mujoe_bsp_is_HV_BLEED_ENABLED( void )
{
	return LL_GPIO_IsOutputPinSet(HVBUS_BLEED_GPIO_Port, HVBUS_BLEED_Pin );

}// mujoe_bsp_is_HV_BLEED_ENABLED

bool mujoe_bsp_ctrl_HV_BLEED( bool enable )
{
	// Active high
	if( enable )
	{
		LL_GPIO_SetOutputPin(HVBUS_BLEED_GPIO_Port, HVBUS_BLEED_Pin);
	}
	else
	{
		LL_GPIO_ResetOutputPin(HVBUS_BLEED_GPIO_Port, HVBUS_BLEED_Pin);
	}

	return ( mujoe_bsp_is_HV_BLEED_ENABLED() == enable );
}// mujoe_bsp_ctrl_HV_BLEED




