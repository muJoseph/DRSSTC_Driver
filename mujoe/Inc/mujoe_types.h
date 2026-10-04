/*
 * mujoe_types.h
 *
 *      Author: Joe Corteo
 */

#ifndef __MUJOE_TYPES_H__
#define __MUJOE_TYPES_H__

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <math.h>
#if PRINTF_ITM_REDIRECT_EN==1
#include <stdio.h>
#endif
#include "stm32g4xx.h"


#include "cmsis_os.h"
////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	MUJOE_ERR_NONE = 0,
	MUJOE_ERR_BUSY = 0xFA,
	MUJOE_ERR_MALLOC_FAILED = 0xFB,
	MUJOE_ERR_NOT_IMPLEMENTED = 0xFC,
	MUJOE_ERR_NOT_SUPPORTED = 0xFD,
	MUJOE_ERR_INV_PARAM = 0xFE,
	MUJOE_ERR_FAILURE = 0xFF,
}mujoe_err_t;

////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////

#define DELAY_MS(MS)				HAL_Delay((MS))

////////////////////////////////////////////////////////////////////////
// Math
////////////////////////////////////////////////////////////////////////

#define CLAMP(X,MIN,MAX)		((X) < (MIN)) ? (MIN) : ((X) > (MAX) ? (MAX) : (X))
#define ROUND_NEAREST(x) ((x) < 0.0 ? ceil((x) - 0.5) : floor((x) + 0.5))

#define MAX(A,B)			(A)>(B) ? (A) : (B)
#define MIN(A,B)			(A)<(B) ? (A) : (B)
////////////////////////////////////////////////////////////////////////
// Debug
////////////////////////////////////////////////////////////////////////

#ifdef MUJOE_DEBUG_EN

#define TRAP_MCU()				while(1)
#define PUT_BREAKPT_HERE()		asm("NOP")

#else

#define TRAP_MCU()
#define PUT_BREAKPT_HERE()

#endif // #ifdef MUJOE_DEBUG_EN

#if PRINTF_ITM_REDIRECT_EN==1

// Define the variadic macro for logging errors and info

// NOTE: printf MUST contain newline for re-direct to ITM to work
#define LOG_ERROR(format, ...) \
    fprintf(stderr, "@ERROR (%s, line %d):: " format "\n", __FILE__, __LINE__, ##__VA_ARGS__)

// NOTE: printf MUST contain newline for re-direct to ITM to work
#define LOG_INFO(format, ...) \
    fprintf(stdout, "@INFO:: " format "\n", ##__VA_ARGS__)

#else
#define LOG_ERROR(E)
#define LOG_INFO(I)
#endif // #if PRINTF_ITM_REDIRECT_EN==1

////////////////////////////////////////////////////////////////////////
// Byte/Bit Manipulation
////////////////////////////////////////////////////////////////////////
#define BV(N)					((uint32_t)(0x00000001)) << (N)		// Bit value

#define GET_XMSB_U32(X)			(uint8_t)((X)>>24)
#define GET_MSB_U32(X)			(uint8_t)((X)>>16)
#define GET_LSB_U32(X)			(uint8_t)((X)>>8)
#define GET_XLSB_U32(X)			(uint8_t)(X)

////////////////////////////////////////////////////////////////////////
// Memory management
////////////////////////////////////////////////////////////////////////

#define MUJOE_MALLOC(N)			pvPortMalloc((N))
#define MUJOE_FREE(PTR)			if((PTR)!= NULL){\
								vPortFree((PTR));\
								(PTR)=NULL;}

////////////////////////////////////////////////////////////////////////
// System
////////////////////////////////////////////////////////////////////////

#define MUJOE_PERFORM_SYS_RESET()		NVIC_SystemReset()

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

#if PRINTF_ITM_REDIRECT_EN==1
extern int _write(int file, char *ptr, int len);
#endif // #ifndef __MUJOE_TYPES_H__

#endif // #ifndef __MUJOE_TYPES_H__
