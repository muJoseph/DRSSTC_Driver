/*
 * mujoe_usart.h
 *
 *  Created on: Oct 11, 2025
 *      Author: Joe
 */

#ifndef MUJOE_USART_H_
#define MUJOE_USART_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "stm32g4xx_ll_usart.h"
#include "mujoe_dma.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

#define NUM_USART_PERIPH			3

typedef enum
{
	USART_CTX_ID_USART1 = (uint8_t)0,
	USART_CTX_ID_USART2,
	USART_CTX_ID_USART3,
	USART_NUM_CTX,
}usart_ctx_id_t;

typedef enum
{
	MUJOE_USART_PRESET_DEFAULT = (uint8_t)0,

}mujoe_usart_preset_id_t;



typedef void (*mujoe_usart_IDLE_Fp_t)( void );	// USART IDLE interrupt
typedef void (*mujoe_usart_RTO_Fp_t)( void );	// USART receiver timeout interrupt (RTO)

typedef struct _mujoe_usart_callbacks
{
	mujoe_usart_IDLE_Fp_t		idleCb;
	mujoe_usart_RTO_Fp_t		rtoCb;

}mujoe_usart_callbacks_t;

typedef struct _usart_ctx_cfg
{
	USART_TypeDef 				*instance;
	DMA_CTX						dmaCtx;
	uint32_t					rxDmaCh;
	uint32_t					txDmaCh;
	mujoe_usart_preset_id_t		presetId;
	uint32_t					fifoTxThresh;
	uint32_t					fifoRxThresh;
	IRQn_Type					irqType;
	uint16_t					txBufferLen;
	uint16_t					rxBufferLen;
	bool						enableRxHalfXfrInt;
	bool						enableIdleInt;
	bool						enableRxTimeoutInt;
}usart_ctx_cfg_t;

typedef struct _usart_ctx
{
	usart_ctx_cfg_t  			cfg;
	mujoe_usart_callbacks_t		*pCbs;
	uint8_t						*pTxBuf;
	uint8_t						*pRxBuf;
	bool						txPending;		// True when USART TX is in progress


}usart_ctx_t, *USART_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////
extern mujoe_err_t mujoe_usart_init_context( USART_CTX ctx, usart_ctx_cfg_t *pCfg );
extern void mujoe_usart_registerCallbacks( USART_CTX ctx, mujoe_usart_callbacks_t *pCbs );
extern mujoe_err_t mujoe_usart_initConfigStruct( mujoe_usart_preset_id_t preset, LL_USART_InitTypeDef *pInit );
extern mujoe_err_t mujoe_usart_initHardware( USART_CTX ctx );

extern mujoe_err_t mujoe_usart_write_DMA( USART_CTX ctx, void* pData, int dataLen );
extern mujoe_err_t mujoe_usart_write_DMA_legacy( USART_CTX ctx, void* pData, int dataLen );
extern mujoe_err_t mujoe_usart_read_DMA( USART_CTX ctx, int bytesToRead );
extern mujoe_err_t mujoe_usart_readRxBuffer( USART_CTX ctx, uint8_t *pBuf, int numBytes);
extern uint8_t* mujoe_usart_getRxBuffer( USART_CTX ctx );
extern void mujoe_usart_updateTxPending( USART_CTX ctx, bool txPending );

extern mujoe_err_t mujoe_usart_ctrlDMAReq_Tx( USART_CTX ctx, bool enable);
extern mujoe_err_t mujoe_usart_cntrlIDLEInterrupt( USART_CTX ctx, bool enable);
extern mujoe_err_t mujoe_usart_cntrlRxTimeoutInterrupt( USART_CTX ctx, bool enable);

extern uint32_t mujoe_usart_getNumRxRemain_DMA( USART_CTX ctx );

extern void mujoe_usart_handle_usartx_irq( usart_ctx_id_t id );

#endif /* MUJOE_USART_H_ */
