/*
 * mujoe_ser_comm.c
 *
 *  Created on: Oct 13, 2025
 *      Author: Joe
 */


////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_ser_comm.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define LITTLE_ENDIAN				true

////////////////////////////////////////////////////////////////////////
// PROTOS
////////////////////////////////////////////////////////////////////////

static uint16_t calculate_crc16_ccitt(const uint8_t *data, size_t length);



////////////////////////////////////////////////////////////////////////
// MACROS
////////////////////////////////////////////////////////////////////////

#define RAISE_ERROR(ERR_ID)			if( ctx->pCbs && ctx->pCbs->errorCb ){\
										ctx->pCbs->errorCb(ctx, (ERR_ID));}

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_ser_comm_initContext( MUJOE_SER_COMM ctx, mujoe_ser_comm_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(mujoe_ser_comm_t));
	ctx->cfg = *pCfg;

	uint32_t min = 0, max = 0;
	mujoe_ser_comm_getMinMaxPayloadLength(&min,&max);
#if RES_DRV_DBG_EN == 1
	LOG_INFO("Mujoe serial comm payload lengths, min: %d max: %d", (int)min, (int)max );
#endif // #if RES_DRV_DBG_EN == 1

	return MUJOE_ERR_NONE;
}// mujoe_hrtim_init_context

void mujoe_ser_comm_registerCallbacks( MUJOE_SER_COMM ctx, mujoe_ser_comm_Cbs_t *pCbs )
{
	ctx->pCbs = pCbs;
}// mujoe_ser_comm_registerCallbacks

void mujoe_ser_comm_registerCmdHdlrs( MUJOE_SER_COMM ctx,
									  mujoe_sc_cmd_handler_t *pHdlrs,
									  int numHdlrs,
									  void *pUArg )
{
	ctx->pCmdTbl = pHdlrs;
	ctx->cmdTblLen = numHdlrs;
	ctx->pCmdTblUArg = pUArg;

}// mujoe_ser_comm_registerCmdHdlrs


void mujoe_ser_comm_setFlags( MUJOE_SER_COMM ctx, sc_flags_t flags )
{
	ctx->flags = flags;
}// mujoe_ser_comm_setFlags

mujoe_err_t mujoe_ser_comm_sendCmdRspFrame( MUJOE_SER_COMM ctx, mujoe_sc_cmd_rsp_t *pCmdRsp )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	uint8_t *pFrame = NULL;
	uint32_t frameLen = 0;

	// Update command response payload with current flags
	pCmdRsp->flags = ctx->flags;

	if( (err = mujoe_ser_comm_createCmdRspFrame( ctx, pCmdRsp, &pFrame, &frameLen )) == MUJOE_ERR_NONE )
	{
		if( ctx->pCbs && ctx->pCbs->writeCb )
		{
			 err = ctx->pCbs->writeCb(ctx->cfg.pHostArg, pFrame, frameLen );
		}
		else
		{
			err = MUJOE_ERR_NOT_IMPLEMENTED;
		}

		MUJOE_FREE(pFrame);
	}


	return err;
}// mujoe_ser_comm_sendCmdRspFrame

mujoe_err_t mujoe_ser_comm_createCmdRspFrame( MUJOE_SER_COMM ctx, mujoe_sc_cmd_rsp_t *pCmdRsp, uint8_t **ppFrame, uint32_t *pFrameLen )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	*pFrameLen = MUJOE_SC_FRAME_BASE_LEN + sizeof(mujoe_sc_cmd_rsp_t);
	uint8_t *pFrame = (uint8_t *)MUJOE_MALLOC(*pFrameLen);

	if( pFrame != NULL )
	{
		int idx = 0;
		memset(pFrame,0,*pFrameLen);

		mujoe_sc_frame_hdr_t hdr =
		{
				.preamble = MUJOE_SC_FRAME_PREAMBLE,
				.id = SC_FRAME_ID_CMD_RSP,
				.payloadLen = sizeof(mujoe_sc_cmd_rsp_t),

		};
		// Copy frame header into frame buffer
		memcpy(&pFrame[idx],&hdr,sizeof(mujoe_sc_frame_hdr_t));
		idx += sizeof(mujoe_sc_frame_hdr_t);
		// Copy payload into frame buffer
		memcpy(&pFrame[idx],pCmdRsp,sizeof(mujoe_sc_cmd_rsp_t));
		idx += sizeof(mujoe_sc_cmd_rsp_t);

		int crcComputeLen = sizeof(sc_frame_id_t) + sizeof(sc_frame_payload_len_t) + sizeof(mujoe_sc_cmd_rsp_t);
		// Comptue crc across frame (excluding preamble)
		uint16_t crc = calculate_crc16_ccitt(&pFrame[sizeof(preamble_t)], crcComputeLen);

		// Copy crc into payload
		memcpy(&pFrame[idx],&crc,sizeof(sc_frame_crc_t));

		// Output pointer to frame
		*ppFrame = pFrame;

	}
	else
		err = MUJOE_ERR_MALLOC_FAILED;

	return err;
}// mujoe_ser_comm_build_cmp_rsp

mujoe_err_t mujoe_ser_comm_sendSolicitedDataFrame( MUJOE_SER_COMM ctx, mujoe_sc_sol_data_t *pSolData )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	uint8_t *pFrame = NULL;
	uint32_t frameLen = 0;

	if( (err = mujoe_ser_comm_createSolicitedDataFrame( ctx, pSolData, &pFrame, &frameLen )) == MUJOE_ERR_NONE )
	{
		if( ctx->pCbs && ctx->pCbs->writeCb )
		{
			 err = ctx->pCbs->writeCb(ctx->cfg.pHostArg, pFrame, frameLen );
		}
		else
		{
			err = MUJOE_ERR_NOT_IMPLEMENTED;
		}

		MUJOE_FREE(pFrame);
	}


	return err;
}// mujoe_ser_comm_sendSolicitedDataFrame

mujoe_err_t mujoe_ser_comm_createSolicitedDataFrame( MUJOE_SER_COMM ctx, mujoe_sc_sol_data_t *pSolData, uint8_t **ppFrame, uint32_t *pFrameLen )
{
	mujoe_err_t err = MUJOE_ERR_NONE;

	*pFrameLen = MUJOE_SC_FRAME_BASE_LEN + sizeof(mujoe_sc_sol_data_t);
	uint8_t *pFrame = (uint8_t *)MUJOE_MALLOC(*pFrameLen);

	if( pFrame != NULL )
	{
		int idx = 0;
		memset(pFrame,0,*pFrameLen);

		mujoe_sc_frame_hdr_t hdr =
		{
				.preamble = MUJOE_SC_FRAME_PREAMBLE,
				.id = SC_FRAME_ID_SOL,
				.payloadLen = sizeof(mujoe_sc_sol_data_t),

		};
		// Copy frame header into frame buffer
		memcpy(&pFrame[idx],&hdr,sizeof(mujoe_sc_frame_hdr_t));
		idx += sizeof(mujoe_sc_frame_hdr_t);
		// Copy payload into frame buffer
		memcpy(&pFrame[idx],pSolData,sizeof(mujoe_sc_sol_data_t));
		idx += sizeof(mujoe_sc_sol_data_t);

		int crcComputeLen = sizeof(sc_frame_id_t) + sizeof(sc_frame_payload_len_t) + sizeof(mujoe_sc_sol_data_t);
		// Comptue crc across frame (excluding preamble)
		uint16_t crc = calculate_crc16_ccitt(&pFrame[sizeof(preamble_t)], crcComputeLen);

		// Copy crc into payload
		memcpy(&pFrame[idx],&crc,sizeof(sc_frame_crc_t));

		// Output pointer to frame
		*ppFrame = pFrame;

	}
	else
		err = MUJOE_ERR_MALLOC_FAILED;

	return err;
}// mujoe_ser_comm_createSolicitedDataFrame

void mujoe_ser_comm_getMinMaxPayloadLength( uint32_t *pMin, uint32_t *pMax )
{
	uint32_t payloadLengths[] =
	{
		sizeof(mujoe_sc_cmd_t),
		sizeof(mujoe_sc_cmd_rsp_t),
	};

	uint32_t numPayloadTypes = sizeof(payloadLengths)/sizeof(uint32_t);

	*pMin = 0xFFFFFFFF;
	*pMax = 0;
	for( uint8_t i = 0; i < numPayloadTypes; i++ )
	{
		*pMax = MAX(payloadLengths[i],*pMax);
		*pMin = MIN(payloadLengths[i],*pMin);
	}
}// mujoe_ser_comm_getMinMaxPayloadLength


#if LITTLE_ENDIAN == true
static const uint8_t preamble[4] =
{
	GET_XLSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_LSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_MSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_XMSB_U32(MUJOE_SC_FRAME_PREAMBLE),

};// preamble


static mujoe_err_t proc_frame( MUJOE_SER_COMM ctx )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	//parser_t *pParser = &ctx->parser;
	rxd_frame_t *pFrame = &ctx->parser.rxdFrame.frame;

	switch( pFrame->rxdFrameId )
	{
		case SC_FRAME_ID_CMD:
		{
			//if( sizeof(mujoe_sc_cmd_t) == pFrame->rxdPayloadLen)
			{
				mujoe_sc_cmd_t *pCmd = (mujoe_sc_cmd_t *)pFrame->payloadBuf;

				// Handle command
				for( uint8_t i = 0; i < ctx->cmdTblLen; i++ )
				{
					if( ctx->pCmdTbl[i].cmdId == pCmd->cmd_id )
					{
						mujoe_sc_cmd_rsp_t cmdRsp;
						bool issuCmdRsp = ctx->pCmdTbl[i].handlerCb(ctx->pCmdTblUArg, pCmd->arg, pCmd->argLen, &cmdRsp );

						// Stuff cmdRsp struct with rxd command and system flags
						cmdRsp.cmd_id = pCmd->cmd_id;
						cmdRsp.flags = ctx->flags;

						// Issue command response
						if( issuCmdRsp )
						{
							err |= mujoe_ser_comm_sendCmdRspFrame( ctx, &cmdRsp );
						}
						break;
					}
				}
			}
			// Handle error
//			else
//			{
//				RAISE_ERROR(MUJOE_SC_ERR_INV_CMD_FRAME);
//			}
			break;
		}
		case SC_FRAME_ID_CMD_RSP:
			break;
		case SC_FRAME_ID_UNSOL:
			break;
		default:
			break;
	}

	return err;
}// proc_frame

mujoe_err_t mujoe_ser_comm_feedParser( MUJOE_SER_COMM ctx, uint8_t *pBuf, int len )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	uint8_t *pHead = pBuf;
	parser_t *pParser = &ctx->parser;
	while( len-- > 0)
	{
		switch(pParser->state)
		{
			// Await preamble
			case SC_PARSE_STATE_IDLE:
				if( *(pHead++) == preamble[pParser->idx++] )
				{
					if( pParser->idx == 4 )
					{
						pParser->state = SC_PARSE_STATE_FRAME_ID;
						pParser->idx = 0;
					}
				}
				else
					pParser->idx = 0;
				break;
			case SC_PARSE_STATE_FRAME_ID:
				// RX frame ID LSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdFrameId = *(pHead++);
					pParser->idx++;
				}
				// RX frame ID MSB
				else
				{
					pParser->rxdFrame.frame.rxdFrameId |= ((uint16_t)*(pHead++)) << 8;
					pParser->state = SC_PARSE_STATE_PAYLOAD_LEN;
					pParser->idx = 0;
				}
				break;
			case SC_PARSE_STATE_PAYLOAD_LEN:
				// RX frame payload len LSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdPayloadLen = *(pHead++);
					pParser->idx++;
				}
				// RX frame payload len MSB
				else
				{
					pParser->rxdFrame.frame.rxdPayloadLen |= ((uint16_t)*(pHead++)) << 8;
					// Handle inv payload length
					if( pParser->rxdFrame.frame.rxdPayloadLen > MAX_PAYLOAD_LEN )
					{
						RAISE_ERROR(MUJOE_SC_ERR_INV_PAYLOAD_LEN);

						// Reset parser
						memset(pParser,0,sizeof(parser_t));
					}
					else
					{
						pParser->state = SC_PARSE_STATE_PAYLOAD;
						pParser->idx = 0;
					}

				}
				break;
			case SC_PARSE_STATE_PAYLOAD:
				pParser->rxdFrame.frame.payloadBuf[pParser->idx++] = *(pHead++);
				if( pParser->idx == pParser->rxdFrame.frame.rxdPayloadLen )
				{
					pParser->state = SC_PARSE_STATE_CRC;
					pParser->idx = 0;
				}
				break;
			case SC_PARSE_STATE_CRC:
				// RX frame CRC LSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdCRC = *(pHead++);
					pParser->idx++;
				}
				// RX frame CRC MSB
				else
				{
					pParser->rxdFrame.frame.rxdCRC |=  ((uint16_t)*(pHead++)) << 8;
					uint16_t crc = calculate_crc16_ccitt((const uint8_t *)pParser->rxdFrame.arr,
														 sizeof(sc_frame_id_t) + sizeof(sc_frame_payload_len_t) + pParser->rxdFrame.frame.rxdPayloadLen );
					if( crc == pParser->rxdFrame.frame.rxdCRC )
					{
						err = proc_frame(ctx);
					}
					// Handle invalid crc
					else
					{
						RAISE_ERROR(MUJOE_SC_ERR_CRC_INV);
					}
					// Reset parser
					memset(pParser,0,sizeof(parser_t));
				}
				break;
			default:
				break;
		}
	}

	return err;

}// mujoe_ser_comm_feedParser

#else // Big-endian

static const uint8_t preamble[4] =
{
	GET_XMSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_MSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_LSB_U32(MUJOE_SC_FRAME_PREAMBLE),
	GET_XLSB_U32(MUJOE_SC_FRAME_PREAMBLE)
};// preamble

mujoe_err_t mujoe_ser_comm_feedParser( MUJOE_SER_COMM ctx, uint8_t *pBuf, int len )
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	uint8_t *pHead = pBuf;
	parser_t *pParser = &ctx->parser;
	while( len-- > 0)
	{
		switch(pParser->state)
		{
			// Await preamble
			case SC_PARSE_STATE_IDLE:
				if( *(pHead++) == preamble[pParser->idx++] )
				{
					if( pParser->idx == 4 )
					{
						pParser->state = SC_PARSE_STATE_FRAME_ID;
						pParser->idx = 0;
					}
				}
				else
					pParser->idx = 0;
				break;
			case SC_PARSE_STATE_FRAME_ID:
				// RX frame ID MSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdFrameId = ((uint16_t)*(pHead++)) << 8;
					pParser->idx++;
				}
				// RX frame ID LSB
				else
				{
					pParser->rxdFrame.frame.rxdFrameId |= *(pHead++);
					pParser->state = SC_PARSE_STATE_PAYLOAD_LEN;
					pParser->idx = 0;
				}
				break;
			case SC_PARSE_STATE_PAYLOAD_LEN:
				// RX frame payload len MSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdPayloadLen = ((uint16_t)*(pHead++)) << 8;
					pParser->idx++;
				}
				// RX frame payload len LSB
				else
				{
					pParser->rxdFrame.frame.rxdPayloadLen |= *(pHead++);
					if( pParser->rxdFrame.frame.rxdPayloadLen > MAX_PAYLOAD_LEN )
					{
						// Handle error
					}
					pParser->state = SC_PARSE_STATE_PAYLOAD;
					pParser->idx = 0;
				}
				break;
			case SC_PARSE_STATE_PAYLOAD:
				pParser->rxdFrame.frame.payloadBuf[pParser->idx++] = *(pHead++);
				if( pParser->idx == pParser->rxdFrame.frame.rxdPayloadLen )
				{
					pParser->state = SC_PARSE_STATE_CRC;
					pParser->idx = 0;
				}
				break;
			case SC_PARSE_STATE_CRC:
				// RX frame CRC MSB
				if( pParser->idx == 0 )
				{
					pParser->rxdFrame.frame.rxdCRC = ((uint16_t)*(pHead++)) << 8;
					pParser->idx++;
				}
				// RX frame CRC LSB
				else
				{
					pParser->rxdFrame.frame.rxdCRC |= *(pHead++);
					uint16_t crc = calculate_crc16_ccitt((const uint8_t *)pParser->rxdFrame.arr,
														 sizeof(sc_frame_id_t) + sizeof(sc_frame_payload_len_t) + pParser->rxdFrame.frame.rxdPayloadLen );
					// TODO: Big-endian frame: compute CRC and compare against rxd CRC
					pParser->state = SC_PARSE_STATE_PAYLOAD_LEN;
					pParser->idx = 0;
				}
				break;
			default:
				break;
		}
	}

	return err;

}// mujoe_ser_comm_feedParser

#endif // #if LITTLE_ENDIAN == true


////////////////////////////////////////////////////////////////////////
// STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////

// CRC-CCITT polynomial
#define POLY 0x1021

// Initial CRC value
#define INIT_CRC 0xFFFF

uint16_t calculate_crc16_ccitt(const uint8_t *data, size_t length)
{
    uint16_t crc = INIT_CRC;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8; // XOR data byte with upper byte of CRC

        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
            { // If MSB is 1
                crc = (crc << 1) ^ POLY; // Shift left and XOR with polynomial
            }
            else
            {
                crc <<= 1; // Shift left
            }
        }
    }
    return crc;
}// calculate_crc16_ccitt
