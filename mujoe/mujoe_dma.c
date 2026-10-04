/*
 * mujoe_dma.c
 *
 *  Created on: Sep 20, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_dma.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

static const DMA_TypeDef* dma_instances[NUM_DMA_MODULES] =
{
		DMA1,
		DMA2,
};

static dma_callbacks_t *pDma1Cbs = NULL;
static dma_callbacks_t *pDma2Cbs = NULL;

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_dma_init_context( DMA_CTX ctx, dma_ctx_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(dma_ctx_t));
	ctx->cfg = *pCfg;
	if( ctx->cfg.dma_id < NUM_DMA_MODULES)
	{
		ctx->instance = (DMA_TypeDef*)dma_instances[ctx->cfg.dma_id];
		return MUJOE_ERR_NONE;
	}
	else
		return MUJOE_ERR_INV_PARAM;

}// mujoe_dma_init_context

void mujoe_dma_registerCallbacks( DMA_CTX ctx, dma_callbacks_t *pCbs )
{

	ctx->pCbs = pCbs;

	if( ctx->cfg.dma_id == DMA_ID_1 )
		pDma1Cbs = pCbs;
	else
		pDma2Cbs = pCbs;

}// mujoe_dma_registerCallbacks

uint32_t mujoe_dma_getDataLength( DMA_CTX ctx, uint32_t ch)
{
	return LL_DMA_GetDataLength(ctx->instance, ch);

}// mujoe_dma_getDataLength


mujoe_err_t mujoe_dma_isTransferComplete( DMA_CTX ctx, uint32_t ch, bool *pComplete )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	switch( ch )
	{
		case LL_DMA_CHANNEL_7:
			*pComplete = LL_DMA_IsActiveFlag_TC7(ctx->instance);
			break;
		default:
			err = MUJOE_ERR_NOT_IMPLEMENTED;
			break;
	}

	return err;

}// mujoe_dma_isTransferComplete

mujoe_err_t mujoe_dma_clearFlag_transferError( DMA_CTX ctx, uint32_t ch )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	DMA_TypeDef *DMA = ctx->instance;
	bool TEx = false;
	switch( ch )
	{
		case LL_DMA_CHANNEL_1:
			LL_DMA_ClearFlag_TE1(DMA);
			TEx = LL_DMA_IsActiveFlag_TE1(DMA);
			break;
		case LL_DMA_CHANNEL_2:
			LL_DMA_ClearFlag_TE2(DMA);
			TEx = LL_DMA_IsActiveFlag_TE2(DMA);
			break;
		case LL_DMA_CHANNEL_3:
			LL_DMA_ClearFlag_TE3(DMA);
			TEx = LL_DMA_IsActiveFlag_TE3(DMA);
			break;
		case LL_DMA_CHANNEL_4:
			LL_DMA_ClearFlag_TE4(DMA);
			TEx = LL_DMA_IsActiveFlag_TE4(DMA);
			break;
		case LL_DMA_CHANNEL_5:
			LL_DMA_ClearFlag_TE5(DMA);
			TEx = LL_DMA_IsActiveFlag_TE5(DMA);
			break;
		case LL_DMA_CHANNEL_6:
			LL_DMA_ClearFlag_TE6(DMA);
			TEx = LL_DMA_IsActiveFlag_TE6(DMA);
			break;
		case LL_DMA_CHANNEL_7:
			LL_DMA_ClearFlag_TE7(DMA);
			TEx = LL_DMA_IsActiveFlag_TE7(DMA);
			break;
		case LL_DMA_CHANNEL_8:
			LL_DMA_ClearFlag_TE8(DMA);
			TEx = LL_DMA_IsActiveFlag_TE8(DMA);
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	if( err == MUJOE_ERR_NONE )
	{
		err = !TEx ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}

	return err;
}// mujoe_dma_clearFlag_transferError

mujoe_err_t mujoe_dma_clearFlag_transferComplete( DMA_CTX ctx, uint32_t ch )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	DMA_TypeDef *DMA = ctx->instance;
	bool TCx = false;
	switch( ch )
	{
		case LL_DMA_CHANNEL_1:
			LL_DMA_ClearFlag_TC1(DMA);
			TCx = LL_DMA_IsActiveFlag_TC1(DMA);
			break;
		case LL_DMA_CHANNEL_2:
			LL_DMA_ClearFlag_TC2(DMA);
			TCx = LL_DMA_IsActiveFlag_TC2(DMA);
			break;
		case LL_DMA_CHANNEL_3:
			LL_DMA_ClearFlag_TC3(DMA);
			TCx = LL_DMA_IsActiveFlag_TC3(DMA);
			break;
		case LL_DMA_CHANNEL_4:
			LL_DMA_ClearFlag_TC4(DMA);
			TCx = LL_DMA_IsActiveFlag_TC4(DMA);
			break;
		case LL_DMA_CHANNEL_5:
			LL_DMA_ClearFlag_TC5(DMA);
			TCx = LL_DMA_IsActiveFlag_TC5(DMA);
			break;
		case LL_DMA_CHANNEL_6:
			LL_DMA_ClearFlag_TC6(DMA);
			TCx = LL_DMA_IsActiveFlag_TC6(DMA);
			break;
		case LL_DMA_CHANNEL_7:
			LL_DMA_ClearFlag_TC7(DMA);
			TCx = LL_DMA_IsActiveFlag_TC7(DMA);
			break;
		case LL_DMA_CHANNEL_8:
			LL_DMA_ClearFlag_TC8(DMA);
			TCx = LL_DMA_IsActiveFlag_TC8(DMA);
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	if( err == MUJOE_ERR_NONE )
	{
		err = !TCx ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	}

	return err;
}// mujoe_dma_clearFlag_transferComplete


mujoe_err_t mujoe_dma_config( DMA_CTX ctx, dma_cfg_t *pCfg)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	DMA_TypeDef *DMA = ctx->instance;

	LL_DMA_DisableChannel(DMA,pCfg->dmaChannel);

	// Ensure that DMA channel is disabled...
	if( LL_DMA_IsEnabledChannel(DMA, pCfg->dmaChannel) == 0 )
	{
		LL_DMA_SetDataTransferDirection(DMA, pCfg->dmaChannel, pCfg->tranferDirection);

		LL_DMA_ConfigAddresses(DMA,
				pCfg->dmaChannel,
				pCfg->SrcAddress,
				pCfg->DstAddress,
				LL_DMA_GetDataTransferDirection(DMA, pCfg->dmaChannel));

		// Set and confirm DMA request ID for DMA MUX
		LL_DMA_SetPeriphRequest(DMA, pCfg->dmaChannel, pCfg->dmaRequestId);
		err |= LL_DMA_GetPeriphRequest(DMA, pCfg->dmaChannel) == pCfg->dmaRequestId ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_SetDataLength(DMA, pCfg->dmaChannel, pCfg->numberOfData);

		LL_DMA_SetChannelPriorityLevel(DMA, pCfg->dmaChannel, pCfg->priority);

		LL_DMA_SetMode(DMA, pCfg->dmaChannel, pCfg->circularModeEn ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL);

		LL_DMA_SetPeriphIncMode(DMA, pCfg->dmaChannel, pCfg->periphIncEn ? LL_DMA_PERIPH_INCREMENT : LL_DMA_PERIPH_NOINCREMENT);

		LL_DMA_SetMemoryIncMode(DMA, pCfg->dmaChannel, pCfg->memIncEn ? LL_DMA_MEMORY_INCREMENT : LL_DMA_MEMORY_NOINCREMENT);

		LL_DMA_SetPeriphSize(DMA, pCfg->dmaChannel, pCfg->periphSize);
		LL_DMA_SetMemorySize(DMA, pCfg->dmaChannel, pCfg->memSize);

		// Enable transfer error interrupt
		if( ctx->pCbs && ctx->pCbs->transferErrorCB )
		{
			// Clear any pending transfer error flag
			err |= mujoe_dma_clearFlag_transferError(ctx, pCfg->dmaChannel ); // TEST
			LL_DMA_EnableIT_TE(DMA, pCfg->dmaChannel);
		}


		// Enable transfer complete interrupt
		if( ctx->pCbs && ctx->pCbs->transferCompleteCB )
		{
			// Clear any pending transfer complete flag
			err |= mujoe_dma_clearFlag_transferComplete( ctx, pCfg->dmaChannel  );	// TEST
			LL_DMA_EnableIT_TC(DMA, pCfg->dmaChannel);
		}


		LL_DMA_EnableChannel(DMA, pCfg->dmaChannel);
		if( err == MUJOE_ERR_NONE )
		{
			err = LL_DMA_IsEnabledChannel(DMA, pCfg->dmaChannel) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
		}
		else
		{
			err = MUJOE_ERR_FAILURE;
		}

	}
	else
		err = MUJOE_ERR_FAILURE;


	return err;
} // mujoe_dma_config

mujoe_err_t mujoe_dma_config_usart1Tx( DMA_TypeDef* DMA,
									   uint32_t dmaCh,
									   uint8_t *pTxBuf,
									   int txLen,
									   bool enInterrupt,
									   bool repeat )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	LL_DMA_DisableChannel(DMA, dmaCh);

	// Ensure that DMA channel is disabled...
	if( LL_DMA_IsEnabledChannel(DMA, dmaCh) == 0 )
	{
		LL_DMA_SetDataTransferDirection(DMA, dmaCh, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

		LL_DMA_ConfigAddresses(DMA,
				dmaCh,
				(uint32_t)pTxBuf,
				LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_TRANSMIT),
				LL_DMA_GetDataTransferDirection(DMA, dmaCh));

		// Set and confirm DMA request ID for DMA MUX
		LL_DMA_SetPeriphRequest(DMA, dmaCh, LL_DMAMUX_REQ_USART1_TX);
		err |= LL_DMA_GetPeriphRequest(DMA, dmaCh) == LL_DMAMUX_REQ_USART1_TX ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_SetDataLength(DMA, dmaCh, txLen);

		LL_DMA_SetChannelPriorityLevel(DMA, dmaCh, LL_DMA_PRIORITY_HIGH);

		LL_DMA_SetMode(DMA, dmaCh, repeat ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL);

		LL_DMA_SetPeriphIncMode(DMA, dmaCh, LL_DMA_PERIPH_NOINCREMENT);

		LL_DMA_SetMemoryIncMode(DMA, dmaCh, LL_DMA_MEMORY_INCREMENT );

		LL_DMA_SetPeriphSize(DMA, dmaCh, LL_DMA_PDATAALIGN_BYTE);
		LL_DMA_SetMemorySize(DMA, dmaCh, LL_DMA_MDATAALIGN_BYTE);


		if( enInterrupt )
		{
			LL_DMA_EnableIT_TE(DMA, dmaCh);	// transfer error
			LL_DMA_EnableIT_TC(DMA, dmaCh);	// transfer complete

		}
		else
		{
			LL_DMA_DisableIT_TE(DMA, dmaCh);
			LL_DMA_DisableIT_TC(DMA, dmaCh);
		}

		LL_DMA_EnableChannel(DMA, dmaCh);
		err |= LL_DMA_IsEnabledChannel(DMA, dmaCh) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	}
	else
		err = MUJOE_ERR_FAILURE;


	return err;
} // mujoe_dma_config_usart1Tx

mujoe_err_t mujoe_dma_config_lite( DMA_TypeDef* DMA, dma_cfg_t *pCfg, bool xfrIntEn )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	LL_DMA_DisableChannel(DMA, pCfg->dmaChannel);

	// Ensure that DMA channel is disabled...
	if( LL_DMA_IsEnabledChannel(DMA, pCfg->dmaChannel) == 0 )
	{
		LL_DMA_SetDataTransferDirection(DMA, pCfg->dmaChannel, pCfg->tranferDirection);

		LL_DMA_ConfigAddresses(DMA,
				pCfg->dmaChannel,
				pCfg->SrcAddress,
				pCfg->DstAddress,
				LL_DMA_GetDataTransferDirection(DMA, pCfg->dmaChannel));

		// Set and confirm DMA request ID for DMA MUX
		LL_DMA_SetPeriphRequest(DMA, pCfg->dmaChannel, pCfg->dmaRequestId);
		err |= LL_DMA_GetPeriphRequest(DMA, pCfg->dmaChannel) == pCfg->dmaRequestId ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		LL_DMA_SetDataLength(DMA, pCfg->dmaChannel, pCfg->numberOfData);

		LL_DMA_SetChannelPriorityLevel(DMA, pCfg->dmaChannel, pCfg->priority);

		LL_DMA_SetMode(DMA, pCfg->dmaChannel, pCfg->circularModeEn ? LL_DMA_MODE_CIRCULAR : LL_DMA_MODE_NORMAL);

		LL_DMA_SetPeriphIncMode(DMA, pCfg->dmaChannel, pCfg->periphIncEn ? LL_DMA_PERIPH_INCREMENT : LL_DMA_PERIPH_NOINCREMENT);

		LL_DMA_SetMemoryIncMode(DMA, pCfg->dmaChannel, pCfg->memIncEn ? LL_DMA_MEMORY_INCREMENT : LL_DMA_MEMORY_NOINCREMENT);

		LL_DMA_SetPeriphSize(DMA, pCfg->dmaChannel, pCfg->periphSize);
		LL_DMA_SetMemorySize(DMA, pCfg->dmaChannel, pCfg->memSize);

		// Enable transfer error interrupt
		if( xfrIntEn )
		{
			LL_DMA_EnableIT_TE(DMA, pCfg->dmaChannel);	// transfer error
			LL_DMA_EnableIT_TC(DMA, pCfg->dmaChannel);	// transfer complete
		}
		else
		{
			LL_DMA_DisableIT_TE(DMA, pCfg->dmaChannel);
			LL_DMA_DisableIT_TC(DMA, pCfg->dmaChannel);
		}

		LL_DMA_EnableChannel(DMA, pCfg->dmaChannel);
		err |= LL_DMA_IsEnabledChannel(DMA, pCfg->dmaChannel) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	}
	else
		err = MUJOE_ERR_FAILURE;


	return err;
} // mujoe_dma_config_lite

bool mujoe_dma_isEnabledChannel( DMA_CTX ctx, uint32_t ch)
{
	return LL_DMA_IsEnabledChannel(ctx->instance, ch);
}// mujoe_dma_isEnabledChannel


////////////////////////////////////////////////////////////////////////
// ISR Handlers
////////////////////////////////////////////////////////////////////////

void mujoe_dma_DMA2_Channel1_IRQHandler(void)
{
	// Check channel 1 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC1(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC1(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(1);
	}
	// Check transfer error flag for channel 1
	else if(LL_DMA_IsActiveFlag_TE1(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(1);
	}

}// mujoe_dma_DMA2_Channel1_IRQHandler

void mujoe_dma_DMA2_Channel2_IRQHandler(void)
{
	// Check channel 2 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC2(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC2(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(2);
	}
	// Check transfer error flag for channel 2
	else if(LL_DMA_IsActiveFlag_TE2(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(2);
	}
}// mujoe_dma_DMA2_Channel2_IRQHandler

void mujoe_dma_DMA2_Channel3_IRQHandler(void)
{
	// Check channel 3 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC3(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC3(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(3);
	}
	// Check transfer error flag for channel 3
	else if(LL_DMA_IsActiveFlag_TE3(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(3);
	}

}// mujoe_dma_DMA2_Channel3_IRQHandler

void mujoe_dma_DMA2_Channel4_IRQHandler(void)
{
	// Check channel 4 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC4(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC4(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(4);
	}
	// Check transfer error flag for channel 4
	else if(LL_DMA_IsActiveFlag_TE4(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(4);
	}
}// mujoe_dma_DMA2_Channel4_IRQHandler

void mujoe_dma_DMA2_Channel5_IRQHandler(void)
{
	// Check channel 5 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC5(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC5(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(5);
	}
	// Check transfer error flag for channel 5
	else if(LL_DMA_IsActiveFlag_TE5(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(5);
	}

}// mujoe_dma_DMA2_Channel5_IRQHandler

void mujoe_dma_DMA2_Channel6_IRQHandler(void)
{
	// Check channel 6 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC6(DMA2) == 1)
	{
		LL_DMA_ClearFlag_TC6(DMA2);

		if( pDma2Cbs && pDma2Cbs->transferCompleteCB )
			pDma2Cbs->transferCompleteCB(6);
	}
	// Check transfer error flag for channel 6
	else if(LL_DMA_IsActiveFlag_TE6(DMA2) == 1)
	{
		if( pDma2Cbs && pDma2Cbs->transferErrorCB )
			pDma2Cbs->transferErrorCB(6);
	}
}// mujoe_dma_DMA2_Channel6_IRQHandler


void mujoe_dma_DMA1_Channel1_IRQHandler(void)
{
	// Check channel 1 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC1(DMA1) == 1)
	{
		LL_DMA_ClearFlag_TC1(DMA1);

		if( pDma1Cbs && pDma1Cbs->transferCompleteCB )
			pDma1Cbs->transferCompleteCB(1);
	}

	// Check if half transfer flag is set
	if( LL_DMA_IsActiveFlag_HT1(DMA1) == 1)
	{
		LL_DMA_ClearFlag_HT1(DMA1);
	}

	// Check transfer error flag for channel 1
	if(LL_DMA_IsActiveFlag_TE1(DMA1) == 1)
	{
		if( pDma1Cbs && pDma1Cbs->transferErrorCB )
			pDma1Cbs->transferErrorCB(1);
	}

}// mujoe_dma_DMA1_Channel1_IRQHandler


void mujoe_dma_DMA1_Channel6_IRQHandler(void)
{
	// Check channel 6 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC6(DMA1) == 1)
	{
		LL_DMA_ClearFlag_TC6(DMA1);

		if( pDma1Cbs && pDma1Cbs->transferCompleteCB )
			pDma1Cbs->transferCompleteCB(6);

		//int dataLeft = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		//LOG_INFO("DMA1 CH6: TC6 intr, num data left: %d",dataLeft);
	}

	// Check if half transfer flag is set
	if( LL_DMA_IsActiveFlag_HT6(DMA1) == 1)
	{
		LL_DMA_ClearFlag_HT6(DMA1);

		//int dataLeft = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		//LOG_INFO("DMA1 CH6: HT6 intr, num data left: %d",dataLeft);
	}

	// Check transfer error flag for channel 6
	if(LL_DMA_IsActiveFlag_TE6(DMA1) == 1)
	{
		if( pDma1Cbs && pDma1Cbs->transferErrorCB )
			pDma1Cbs->transferErrorCB(6);
	}
}// mujoe_dma_DMA1_Channel6_IRQHandler

void mujoe_dma_DMA1_Channel7_IRQHandler(void)
{
	// Check channel 7 transfer complete flag
	if(LL_DMA_IsActiveFlag_TC7(DMA1) == 1)
	{
		LL_DMA_ClearFlag_TC7(DMA1);

		if( pDma1Cbs && pDma1Cbs->transferCompleteCB )
			pDma1Cbs->transferCompleteCB(7);
	}

	// Check if half transfer flag is set
	if( LL_DMA_IsActiveFlag_HT7(DMA1) == 1)
	{
		LL_DMA_ClearFlag_HT7(DMA1);
	}

	// Check transfer error flag for channel 7
	if(LL_DMA_IsActiveFlag_TE7(DMA1) == 1)
	{
		if( pDma1Cbs && pDma1Cbs->transferErrorCB )
			pDma1Cbs->transferErrorCB(7);
	}
}// mujoe_dma_DMA1_Channel7_IRQHandler

