/*
 * mujoe_settings.h
 *
 *  Created on: Oct 11, 2025
 *      Author: Joe
 */

#ifndef MUJOE_SETTINGS_H_
#define MUJOE_SETTINGS_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "cmsis_os2.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "FreeRTOSConfig.h"

////////////////////////////////////////////////////////////////////////
// RTOS Settings
////////////////////////////////////////////////////////////////////////

// Resonant Drive Task
#define RESDRIVE_TASK_PRIORITY			osPriorityAboveNormal
#define RESDRIVE_TASK_STACK_SIZE		256*4	// words (e.g. 128 words = 1024 bytes)
#define RESDRIVE_TASK_QUEUE_MAX_MSGS	8		// Max number of queue msgs in FIFO

// Comm Task
#define COMM_TASK_PRIORITY				osPriorityNormal
#define COMM_TASK_STACK_SIZE			256*4	// words (e.g. 128 words = 1024 bytes)
#define COMM_TASK_QUEUE_MAX_MSGS		8		// Max number of queue msgs in FIFO


#define MAX_TASK_QUEUE_MSG_LEN			64	// Max length of each task queue message

////////////////////////////////////////////////////////////////////////
// Resonant Driver Settings
////////////////////////////////////////////////////////////////////////

// If true, host-specified starting/open-loop frequency for burst generation
// is reloaded on every PRF cycle.
#define ENABLE_BURST_RELOAD				true

#define ENABLE_DMA_CALLBACKS			true  // NOTE: Must be true for finite cycles to work
#define ENABLE_TF2_PIN_OUTPUT			true
#define ENABLE_PULSE_SKIPPING			true


// The amount of time BEFORE the PRF burst completes (in usec)
// that the DMA2 CH6 dma request will be generated for reloading the starting frequency
// into TIM2.ARR (if ENABLE_BURST_RELOAD = true)
#define BURST_DMARQ_LEADTIME_US			100


#define RES_DRIVER_PRESET				RESDRV_PRESET_ID_HIGH_PRI_CURR_STRESS_TEST



#define PRF_FREQ_MIN_HZ					20.0f			// Min Pulse Repetition Frequency
#define PRF_FREQ_MAX_HZ					500.0f			// Max Pulse Repetition Frequency

#define PRF_ON_TIME_MIN_USEC			10				// Min PRF on-time (us)
#define PRF_ON_TIME_MAX_USEC			300				// Max PRF on-time (us)

#define LEAD_TIME_MIN_NSEC				0				// Min lead time (ns)
#define LEAD_TIME_MAX_NSEC				10000			// Max lead time (ns)

#define START_FREQ_MIN_HZ				50e3			// Min Start Frequency (Hz)
#define START_FREQ_MAX_HZ				300e3			// Max Start frequency (Hz)

#define OCD_LIMIT_MIN_A					50.0f			// Min OCD limit (A)
#define OCD_LIMIT_MAX_A					800.0f			// Max OCD limit (A)

#define BURST_SEQUENCE_MAX_DURATION_MS	5000			// 5 sec

#define PRF_MAX_DUTY_CYCLE				0.08f			// Max PRF duty cycle (8%)


#define HBRIDGE_LEAD_TIME_NS			(uint32_t)0		// Default lead time on POR


#define START_FREQ_HZ					264e3			// Start burst frequency excitation (Hz)


#define MUJOE_RES_PER_CAP_BUF_LEN		4700			// Buffer length for resonant period capture (number of uint32_t)

////////////////////////////////////////////////////////////////////////
// MUJOE Serial Comm
////////////////////////////////////////////////////////////////////////

#define MUJOE_FW_VER_MAJOR				(uint8_t)1		// Firmware version
#define MUJOE_FW_VER_MINOR				(uint8_t)0

#define MUJOE_HW_VER_MAJOR				(uint8_t)1		// Hardware version
#define MUJOE_HW_VER_MINOR				(uint8_t)0

#define MUJOE_SC_VER_MAJOR				(uint8_t)1		// Mujoe Serial Comm version
#define MUJOE_SC_VER_MINOR				(uint8_t)0


// Frame payload parser buffer length and thus determines the max
// size of the frame payload
#define MUJOE_SERIAL_COMM_PARSER_PAYLOAD_BUFF_LEN		100

#define MUJOE_SC_MAX_CMD_ARG_LEN						40
#define MUJOE_SC_MAX_CMD_RSP_PARAM_LEN					40
#define MUJOE_SC_MAX_SOL_DATA_CHUNK_LEN					40

// Delay before resonant period capture buffer is dumped to UART once command is rxd
#define MUJOE_SC_RES_PER_CAP_BUF_DMP_DELAY_MS			0

////////////////////////////////////////////////////////////////////////
// USART1
////////////////////////////////////////////////////////////////////////

// TODO: Make USART TX/RX buffer sizes dependent on mujoe_ser_comm max frame lengths
#define USART1_TX_BUFF_LEN								100
#define USART1_RX_BUFF_LEN								100


#define USART1_DMA_RX_LEN								10

#define USART1_DMA_RX_CH								LL_DMA_CHANNEL_6
#define USART1_DMA_TX_CH								LL_DMA_CHANNEL_7

////////////////////////////////////////////////////////////////////////
// ADC2
////////////////////////////////////////////////////////////////////////

// Currently not utilized
#define ADC2_DMA										DMA1
#define ADC2_DMA_CH										LL_DMA_CHANNEL_1

////////////////////////////////////////////////////////////////////////
// System Settings
////////////////////////////////////////////////////////////////////////

#define KERNEL_START_TO_INIT_HW_DELAY_MS				200
#define SYS_RESET_DELAY_MS								1000

#endif /* MUJOE_SETTINGS_H_ */
