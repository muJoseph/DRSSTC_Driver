/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_comp.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_dac.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_i2c.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void TransferComplete_Callback(void);
void TransferError_Callback(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_START_FREQ 180000
#define MASTER_FREQ_HZ 170000000
#define TIM5_PSC 170
#define DRV_START_FREQ_HZ 180000
#define HRTIM_TIMER_PERIOD 0xFFDF
#define TIM2_TRGO_BLANKING 5
#define TIM5_ARR 10
#define HV_ON_Pin LL_GPIO_PIN_13
#define HV_ON_GPIO_Port GPIOC
#define RCC_OSC32_IN_Pin LL_GPIO_PIN_14
#define RCC_OSC32_IN_GPIO_Port GPIOC
#define RCC_OSC32_OUT_Pin LL_GPIO_PIN_15
#define RCC_OSC32_OUT_GPIO_Port GPIOC
#define RCC_OSC_IN_Pin LL_GPIO_PIN_0
#define RCC_OSC_IN_GPIO_Port GPIOF
#define RCC_OSC_OUT_Pin LL_GPIO_PIN_1
#define RCC_OSC_OUT_GPIO_Port GPIOF
#define BUZZER_Pin LL_GPIO_PIN_0
#define BUZZER_GPIO_Port GPIOC
#define LED_Pin LL_GPIO_PIN_1
#define LED_GPIO_Port GPIOC
#define HVBUS_ICL_BYPASS_Pin LL_GPIO_PIN_2
#define HVBUS_ICL_BYPASS_GPIO_Port GPIOC
#define KILL_SW_Pin LL_GPIO_PIN_3
#define KILL_SW_GPIO_Port GPIOC
#define PA3_TP_Pin LL_GPIO_PIN_3
#define PA3_TP_GPIO_Port GPIOA
#define PA5_TP_Pin LL_GPIO_PIN_5
#define PA5_TP_GPIO_Port GPIOA
#define PB10_TP_Pin LL_GPIO_PIN_10
#define PB10_TP_GPIO_Port GPIOB
#define GDT_EN_Pin LL_GPIO_PIN_9
#define GDT_EN_GPIO_Port GPIOC
#define GDT_INT_Pin LL_GPIO_PIN_12
#define GDT_INT_GPIO_Port GPIOA
#define T_SWDIO_Pin LL_GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin LL_GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define PGOOD_Pin LL_GPIO_PIN_2
#define PGOOD_GPIO_Port GPIOD
#define T_SWO_Pin LL_GPIO_PIN_3
#define T_SWO_GPIO_Port GPIOB
#define PB5_TP_Pin LL_GPIO_PIN_5
#define PB5_TP_GPIO_Port GPIOB
#define SD_DETECT_Pin LL_GPIO_PIN_6
#define SD_DETECT_GPIO_Port GPIOB
#define HVBUS_BLEED_Pin LL_GPIO_PIN_9
#define HVBUS_BLEED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
