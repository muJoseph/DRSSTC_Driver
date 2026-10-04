/*
 * mujoe_gpio.c
 *
 *  Created on: Sep 30, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_gpio.h"




static const GPIO_TypeDef* gpio_ports[GPIO_NUM_PORTS] =
{
		GPIOA,
		GPIOB,
		GPIOC,
		GPIOD,
		GPIOE,
		GPIOF,
		GPIOG,
};
////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_gpio_set_inputPullup( GPIO_TypeDef *GPIO_PORT, uint32_t pinMask )
{
	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	  GPIO_InitStruct.Pin = pinMask;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	  GPIO_InitStruct.Alternate = 0;	// don't care
	  ErrorStatus stat = LL_GPIO_Init(GPIO_PORT, &GPIO_InitStruct);
	  return stat == SUCCESS ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_gpio_set_inputPullup


