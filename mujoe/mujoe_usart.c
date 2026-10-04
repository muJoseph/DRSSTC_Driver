/*
 * mujoe_usart.c
 *
 *  Created on: Oct 11, 2025
 *      Author: Joe
 */

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_usart.h"
#include <string.h>
#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////
static USART_CTX _usart_ctxs[USART_NUM_CTX] =
{
		NULL,		// USART1
		NULL,		// USART2
		NULL,		// USARt3
};
////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_usart_init_context( USART_CTX ctx, usart_ctx_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(usart_ctx_t));
	ctx->cfg = *pCfg;
	if( ctx->cfg.instance == USART1 )
	{
		_usart_ctxs[USART_CTX_ID_USART1] = ctx;
	}
	return MUJOE_ERR_NONE;

}// mujoe_usart_init_context

void mujoe_usart_registerCallbacks( USART_CTX ctx, mujoe_usart_callbacks_t *pCbs )
{
	ctx->pCbs = pCbs;
}// mujoe_usart_registerCallbacks

void mujoe_usart_updateTxPending( USART_CTX ctx, bool txPending )
{
	ctx->txPending = txPending;
}// mujoe_usart_updateTxPending

mujoe_err_t mujoe_usart_initConfigStruct( mujoe_usart_preset_id_t preset, LL_USART_InitTypeDef *pInit )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	LL_USART_StructInit(pInit);

	switch (preset)
	{
		case MUJOE_USART_PRESET_DEFAULT:
			pInit->PrescalerValue = LL_USART_PRESCALER_DIV1;
			pInit->BaudRate = 115200;
			pInit->DataWidth = LL_USART_DATAWIDTH_8B;
			pInit->StopBits = LL_USART_STOPBITS_1;
			pInit->Parity = LL_USART_PARITY_NONE;
			pInit->TransferDirection = LL_USART_DIRECTION_TX_RX;
			pInit->HardwareFlowControl = LL_USART_HWCONTROL_NONE;
			pInit->OverSampling = LL_USART_OVERSAMPLING_16;
			break;
		default:
			err = MUJOE_ERR_INV_PARAM;
			break;
	}

	return err;

}// mujoe_usart_initConfigStruct

static mujoe_err_t init_dma(  USART_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	// USART TX DMA Config
	{

		dma_cfg_t cfg =
		{
			.dmaChannel 		= ctx->cfg.txDmaCh,
			.dmaRequestId 		= LL_DMAMUX_REQ_USART1_TX,
			.tranferDirection	= LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
			.SrcAddress 		= (uint32_t)ctx->pTxBuf,
			.DstAddress 		= LL_USART_DMA_GetRegAddr(ctx->cfg.instance, LL_USART_DMA_REG_DATA_TRANSMIT),					// TIM 2 ARR
			.numberOfData 		= 1,							//  Will be updated per transaction
			.periphSize 		= LL_DMA_PDATAALIGN_BYTE,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_BYTE,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= false,						// normal mode
			.periphIncEn		= false,
			.memIncEn			= true,

		};

		err |= mujoe_dma_config(ctx->cfg.dmaCtx, &cfg);

		// Disable again since mujoe_dma_config enables by default
		LL_DMA_DisableChannel(ctx->cfg.dmaCtx->instance, ctx->cfg.txDmaCh);


		// Enable DMA TX Interrupt
		err |= mujoe_usart_ctrlDMAReq_Tx( ctx, true);

		//LL_USART_EnableDMAReq_TX(ctx->cfg.instance);
		//err |= LL_USART_IsEnabledDMAReq_TX(ctx->cfg.instance) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

	}

	// USART RX DMA Config
	{
		dma_cfg_t cfg =
		{
			.dmaChannel 		= ctx->cfg.rxDmaCh,
			.dmaRequestId 		= LL_DMAMUX_REQ_USART1_RX,
			.tranferDirection	= LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
			.SrcAddress 		= LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_RECEIVE),
			.DstAddress 		= (uint32_t)ctx->pRxBuf,
			.numberOfData 		= 1,							//  Will be updated per transaction
			.periphSize 		= LL_DMA_PDATAALIGN_BYTE,		// Size of data on peripheral side
			.memSize 			= LL_DMA_MDATAALIGN_BYTE,		// Size of data on memory side
			.priority			= LL_DMA_PRIORITY_HIGH,
			.circularModeEn		= true,
			.periphIncEn		= false,
			.memIncEn			= true,

		};

		err |= mujoe_dma_config(ctx->cfg.dmaCtx, &cfg);

		if( ctx->cfg.enableRxHalfXfrInt )
		{
			// Enable half transfer interrupt
			LL_DMA_EnableIT_HT(ctx->cfg.dmaCtx->instance, ctx->cfg.rxDmaCh);
			err |= LL_DMA_IsEnabledIT_HT(ctx->cfg.dmaCtx->instance, ctx->cfg.rxDmaCh) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		}

		// Disable again since mujoe_dma_config enables by default
		LL_DMA_DisableChannel(ctx->cfg.dmaCtx->instance, ctx->cfg.rxDmaCh);
	}

	return err;

}// init_dma

mujoe_err_t mujoe_usart_initHardware( USART_CTX ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	ErrorStatus stat = SUCCESS;
	USART_TypeDef* usart = ctx->cfg.instance;

	ctx->pTxBuf = (uint8_t *)MUJOE_MALLOC(ctx->cfg.txBufferLen);
	ctx->pRxBuf = (uint8_t *)MUJOE_MALLOC(ctx->cfg.rxBufferLen);

	if( (ctx->pTxBuf == NULL) || (ctx->pRxBuf == NULL) )
	{
		MUJOE_FREE(ctx->pTxBuf);
		MUJOE_FREE(ctx->pRxBuf);
		return MUJOE_ERR_MALLOC_FAILED;
	}

	err |= init_dma(ctx);

	// Enable/disable USART IDLE interrupt
	err |= mujoe_usart_cntrlIDLEInterrupt(ctx, ctx->cfg.enableIdleInt );
	// Enable/disable USART RX timeout interrupt
	err |= mujoe_usart_cntrlRxTimeoutInterrupt(ctx, ctx->cfg.enableRxTimeoutInt );

	// Clear pending interrupts
	NVIC_ClearPendingIRQ(ctx->cfg.irqType);	// TEST

	// Configure interrupt priority
#if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)
	// Must be of lower priority than configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (numerically higher)
	NVIC_SetPriority(ctx->cfg.irqType, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1 );
#else
	NVIC_SetPriority(ctx->cfg.irqType, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
#endif // #if defined(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY)

	// USARTx interrupt init
	NVIC_EnableIRQ(ctx->cfg.irqType);

	LL_USART_InitTypeDef USART_InitStruct;
	mujoe_usart_initConfigStruct( ctx->cfg.presetId, &USART_InitStruct );

	LL_USART_Disable(usart);
	if( LL_USART_IsEnabled(usart) == 1 )
	{
		return MUJOE_ERR_FAILURE;
	}

	stat |= LL_USART_Init(usart, &USART_InitStruct);
	err |= stat == SUCCESS ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
	// TODO: Consider removing calls to FIFO configuraton since we are using DMA
	LL_USART_SetTXFIFOThreshold(usart, ctx->cfg.fifoTxThresh);
	LL_USART_SetRXFIFOThreshold(usart, ctx->cfg.fifoRxThresh);
	LL_USART_DisableFIFO(usart);
	LL_USART_ConfigAsyncMode(usart);

	LL_USART_Enable(usart);

	/* Polling USART1 initialisation */
	while((!(LL_USART_IsActiveFlag_TEACK(usart))) || (!(LL_USART_IsActiveFlag_REACK(usart))))
	{
	}

	return err;
}// mujoe_usart_initHardware

mujoe_err_t mujoe_usart_ctrlDMAReq_Tx( USART_CTX ctx, bool enable)
{
	if( enable )
	{
		LL_USART_EnableDMAReq_TX(ctx->cfg.instance);
	}
	else
	{
		LL_USART_DisableDMAReq_TX(ctx->cfg.instance);
	}

	return (LL_USART_IsEnabledDMAReq_TX(ctx->cfg.instance) == enable) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_usart_ctrlDMAReq_Tx


mujoe_err_t mujoe_usart_cntrlIDLEInterrupt( USART_CTX ctx, bool enable)
{
	if( enable )
	{
		LL_USART_EnableIT_IDLE(ctx->cfg.instance);
	}
	else
	{
		LL_USART_DisableIT_IDLE(ctx->cfg.instance);
	}

	LL_USART_ClearFlag_IDLE(ctx->cfg.instance);

	return (LL_USART_IsEnabledIT_IDLE(ctx->cfg.instance) == enable) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_usart_cntrlIDLEInterrupt

mujoe_err_t mujoe_usart_cntrlRxTimeoutInterrupt( USART_CTX ctx, bool enable)
{
	if( enable )
	{
		LL_USART_EnableIT_RTO(ctx->cfg.instance);
	}
	else
	{
		LL_USART_DisableIT_RTO(ctx->cfg.instance);
	}

	LL_USART_ClearFlag_RTO(ctx->cfg.instance);

	return (LL_USART_IsEnabledIT_RTO(ctx->cfg.instance) == enable) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

}// mujoe_usart_cntrlRxTimeoutInterrupt

mujoe_err_t mujoe_usart_readRxBuffer( USART_CTX ctx, uint8_t *pBuf, int numBytes)
{
	if( numBytes <= ctx->cfg.rxBufferLen )
	{
		memcpy(pBuf,ctx->pRxBuf,numBytes);
		return MUJOE_ERR_NONE;
	}
	else
		return MUJOE_ERR_INV_PARAM;
}// mujoe_usart_readRxBuffer

uint8_t* mujoe_usart_getRxBuffer( USART_CTX ctx )
{
	return ctx->pRxBuf;
}// mujoe_usart_getRxBuffer

// Get number of bytes that remain to be received in DMA
uint32_t mujoe_usart_getNumRxRemain_DMA( USART_CTX ctx )
{
	return mujoe_dma_getDataLength( ctx->cfg.dmaCtx, ctx->cfg.rxDmaCh);
}// mujoe_usart_getNumRxRemain_DMA

mujoe_err_t mujoe_usart_read_DMA( USART_CTX ctx, int bytesToRead )
{
	mujoe_err_t err = MUJOE_ERR_FAILURE;

	if( bytesToRead > ctx->cfg.rxBufferLen )
		return MUJOE_ERR_INV_PARAM;

	// Ensure that no Rx transaction is ongoing
	if( !mujoe_dma_isEnabledChannel( ctx->cfg.dmaCtx, ctx->cfg.rxDmaCh ) )
	{
		  //memcpy(ctx->pTxBuf, pData, dataLen );

		  // Set transmit length
		  LL_DMA_SetDataLength(ctx->cfg.dmaCtx->instance, ctx->cfg.rxDmaCh, bytesToRead);

		  /* Enable DMA RX Interrupt */
		  LL_USART_EnableDMAReq_RX(ctx->cfg.instance);
		  err = LL_USART_IsEnabledDMAReq_RX(ctx->cfg.instance) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		  /* Enable DMA Channel Rx */
		  LL_DMA_EnableChannel(ctx->cfg.dmaCtx->instance, ctx->cfg.rxDmaCh);
	}

	return err;
}// mujoe_usart_read_DMA

mujoe_err_t mujoe_usart_write_DMA( USART_CTX ctx, void* pData, int dataLen )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	if( dataLen > ctx->cfg.txBufferLen )
		return MUJOE_ERR_INV_PARAM;

	// Ensure that no TX transaction is ongoing
	if( !ctx->txPending )
	{
		  memcpy(ctx->pTxBuf, pData, dataLen );

		  if( ctx->cfg.instance == USART1 )
		  {
			  if( (err |= mujoe_dma_config_usart1Tx( ctx->cfg.dmaCtx->instance,
					  	  	  	  	  	  	    USART1_DMA_TX_CH,
												ctx->pTxBuf,
												dataLen,
			  									true,
			  									false ) ) == MUJOE_ERR_NONE )
			  {
				  ctx->txPending = true;
		      }


		  }
		  else
		  {
			  err = MUJOE_ERR_NOT_SUPPORTED;
		  }

	}
	// Transaction still pending
	else
		err = MUJOE_ERR_FAILURE;

	return err;
}// mujoe_usart_write_DMA

// Obsolete, do not use
mujoe_err_t mujoe_usart_write_DMA_legacy( USART_CTX ctx, void* pData, int dataLen )
{
	mujoe_err_t err = MUJOE_ERR_FAILURE;

	if( dataLen > ctx->cfg.txBufferLen )
		return MUJOE_ERR_INV_PARAM;

	// Ensure that no TX transaction is ongoing
	//if( !mujoe_dma_isEnabledChannel( ctx->cfg.dmaCtx, ctx->cfg.txDmaCh ) )
	if( !ctx->txPending )
	{
		  memcpy(ctx->pTxBuf, pData, dataLen );

		  // Set transmit length
		  LL_DMA_SetDataLength(ctx->cfg.dmaCtx->instance, ctx->cfg.txDmaCh, dataLen);

		  /* Enable DMA TX Interrupt */
		  LL_USART_EnableDMAReq_TX(ctx->cfg.instance);
		  err = LL_USART_IsEnabledDMAReq_TX(ctx->cfg.instance) ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;

		  ctx->txPending = true;
		  /* Enable DMA Channel Tx */
		  LL_DMA_EnableChannel(ctx->cfg.dmaCtx->instance, ctx->cfg.txDmaCh);
	}

	return err;
}// mujoe_usart_write_DMA_legacy

void mujoe_usart_handle_usartx_irq( usart_ctx_id_t id )
{
	//USART_TypeDef* USARTx = _usart_ctxs[id]->cfg.instance;

	if( LL_USART_IsActiveFlag_IDLE( _usart_ctxs[id]->cfg.instance) )
	{
		LL_USART_ClearFlag_IDLE(_usart_ctxs[id]->cfg.instance);
		if(  _usart_ctxs[id]->pCbs && _usart_ctxs[id]->pCbs->idleCb )
		{
			_usart_ctxs[id]->pCbs->idleCb();
		}

	}

	if( LL_USART_IsActiveFlag_RTO( _usart_ctxs[id]->cfg.instance) )
	{
		LL_USART_ClearFlag_RTO(_usart_ctxs[id]->cfg.instance);
		if(  _usart_ctxs[id]->pCbs && _usart_ctxs[id]->pCbs->rtoCb )
		{
			_usart_ctxs[id]->pCbs->rtoCb();
		}
	}
}// mujoe_usart_handle_usartx_irq


