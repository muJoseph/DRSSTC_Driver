/*
 * mujoe_dma.h
 *
 *  Created on: Sep 20, 2025
 *      Author: Joe
 */

#ifndef MUJOE_DMA_H_
#define MUJOE_DMA_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "dma.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////


typedef enum
{
	DMA_ID_1 = 0,
	DMA_ID_2,
	NUM_DMA_MODULES,

}dma_id_t;

typedef void (*transferComplete_Fp_t)(uint8_t dmaCh);
typedef void (*transferError_Fp_t)(uint8_t dmaCh);

typedef struct _dma_callbacks
{
	transferComplete_Fp_t transferCompleteCB;
	transferError_Fp_t	  transferErrorCB;

}dma_callbacks_t;


typedef struct _dma_ctx_cfg
{
	dma_id_t		dma_id;

}dma_ctx_cfg_t;

typedef struct _dma_ctx
{
	dma_ctx_cfg_t  		cfg;
	DMA_TypeDef			*instance;
	dma_callbacks_t		*pCbs;
}dma_ctx_t, *DMA_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_dma_init_context( DMA_CTX ctx, dma_ctx_cfg_t *pCfg );
extern void mujoe_dma_registerCallbacks( DMA_CTX ctx, dma_callbacks_t *pCbs );

typedef struct _dma_cfg
{
	uint32_t dmaChannel;
	uint32_t dmaRequestId;
	uint32_t tranferDirection;
	uint32_t SrcAddress;
	uint32_t DstAddress;
	uint32_t numberOfData;	// Total number of data to transfer
	uint32_t periphSize;	// Size of data on peripheral side
	uint32_t memSize;		// Size of data on memory side
	bool	 periphIncEn;
	bool	 memIncEn;
	bool	 circularModeEn;
	uint32_t priority;

}dma_cfg_t;
extern mujoe_err_t mujoe_dma_config( DMA_CTX ctx, dma_cfg_t *pCfg);
extern mujoe_err_t mujoe_dma_config_lite( DMA_TypeDef* DMA, dma_cfg_t *pCfg, bool xfrIntEn );
extern mujoe_err_t mujoe_dma_config_usart1Tx( DMA_TypeDef* DMA,
									   uint32_t dmaCh,
									   uint8_t *pTxBuf,
									   int txLen,
									   bool enInterrupt,
									   bool repeat );


extern mujoe_err_t mujoe_dma_isTransferComplete( DMA_CTX ctx, uint32_t ch, bool *pComplete );

extern bool mujoe_dma_isEnabledChannel( DMA_CTX ctx, uint32_t ch);

extern uint32_t mujoe_dma_getDataLength( DMA_CTX ctx, uint32_t ch);

extern mujoe_err_t mujoe_dma_clearFlag_transferError( DMA_CTX ctx, uint32_t ch );
extern mujoe_err_t mujoe_dma_clearFlag_transferComplete( DMA_CTX ctx, uint32_t ch );

// ISR Handlers
extern void mujoe_dma_DMA2_Channel1_IRQHandler(void);
extern void mujoe_dma_DMA2_Channel2_IRQHandler(void);
extern void mujoe_dma_DMA2_Channel3_IRQHandler(void);
extern void mujoe_dma_DMA2_Channel4_IRQHandler(void);
extern void mujoe_dma_DMA2_Channel5_IRQHandler(void);
extern void mujoe_dma_DMA2_Channel6_IRQHandler(void);

extern void mujoe_dma_DMA1_Channel1_IRQHandler(void);
extern void mujoe_dma_DMA1_Channel6_IRQHandler(void);
extern void mujoe_dma_DMA1_Channel7_IRQHandler(void);



#endif /* MUJOE_DMA_H_ */
