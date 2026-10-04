/*
 * mujoe_ser_comm.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Joe
 */

#ifndef MUJOE_SER_COMM_H_
#define MUJOE_SER_COMM_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////


#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_ser_comm_types.h"
#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////

#define MAX_PAYLOAD_LEN			MUJOE_SERIAL_COMM_PARSER_PAYLOAD_BUFF_LEN

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	MUJOE_SC_ERR_CRC_INV = (uint8_t)1,
	MUJOE_SC_ERR_INV_PAYLOAD_LEN = (uint8_t)2,
	MUJOE_SC_ERR_INV_CMD_FRAME = (uint8_t)3,

}mujoe_sc_err_t;

typedef mujoe_err_t (*mujoe_ser_comm_write_Fp_t)(void *hostArg, void *pBuf, int len );
typedef void (*mujoe_ser_comm_error_Fp_t)(void *hostArg, mujoe_sc_err_t err );

typedef struct _mujoe_ser_comm_Cbs
{
	mujoe_ser_comm_write_Fp_t		writeCb;
	mujoe_ser_comm_error_Fp_t		errorCb;

}mujoe_ser_comm_Cbs_t;


typedef struct _mujoe_ser_comm_cfg
{
	void* 		pHostArg;

}mujoe_ser_comm_cfg_t;

typedef enum
{
	SC_PARSE_STATE_IDLE = (uint8_t)0,
	SC_PARSE_STATE_FRAME_ID,
	SC_PARSE_STATE_PAYLOAD_LEN,
	SC_PARSE_STATE_PAYLOAD,
	SC_PARSE_STATE_CRC,
}sc_parser_state_t;


typedef struct _rxd_frame
{
	sc_frame_id_t			rxdFrameId;
	sc_frame_payload_len_t	rxdPayloadLen;
	uint8_t					payloadBuf[MAX_PAYLOAD_LEN];
	uint16_t				rxdCRC;
}rxd_frame_t;

#define MAX_FRAME_LEN				sizeof(rxd_frame_t)

typedef union _rxd_frame_u
{
	rxd_frame_t		frame;
	uint8_t			arr[MAX_FRAME_LEN];

}rxd_frame_u_t;

typedef struct _parser
{
	sc_parser_state_t		state;
	int						idx;
	rxd_frame_u_t			rxdFrame;
}parser_t;

typedef struct _mujoe_ser_comm
{
	mujoe_ser_comm_cfg_t	cfg;
	mujoe_ser_comm_Cbs_t	*pCbs;
	mujoe_sc_cmd_handler_t 	*pCmdTbl;
	int						cmdTblLen;
	void					*pCmdTblUArg;
	parser_t				parser;
	sc_flags_t				flags;		// Current user flags

}mujoe_ser_comm_t, *MUJOE_SER_COMM;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_ser_comm_initContext( MUJOE_SER_COMM ctx, mujoe_ser_comm_cfg_t *pCfg );
extern void mujoe_ser_comm_registerCallbacks( MUJOE_SER_COMM ctx, mujoe_ser_comm_Cbs_t *pCbs );
extern void mujoe_ser_comm_registerCmdHdlrs( MUJOE_SER_COMM ctx,
											  mujoe_sc_cmd_handler_t *pHdlrs,
											  int numHdlrs,
											  void *pUArg );
extern void mujoe_ser_comm_setFlags( MUJOE_SER_COMM ctx, sc_flags_t flags );
extern mujoe_err_t mujoe_ser_comm_feedParser( MUJOE_SER_COMM ctx, uint8_t *pBuf, int len );
extern void mujoe_ser_comm_getMinMaxPayloadLength( uint32_t *pMin, uint32_t *pMax );

extern mujoe_err_t mujoe_ser_comm_createCmdRspFrame( MUJOE_SER_COMM ctx,
													mujoe_sc_cmd_rsp_t *pCmdRsp,
													uint8_t **ppFrame,
													uint32_t *pFrameLen );
extern mujoe_err_t mujoe_ser_comm_sendCmdRspFrame( MUJOE_SER_COMM ctx, mujoe_sc_cmd_rsp_t *pCmdRsp );

extern mujoe_err_t mujoe_ser_comm_createSolicitedDataFrame( MUJOE_SER_COMM ctx,
															mujoe_sc_sol_data_t *pSolData,
															uint8_t **ppFrame,
															uint32_t *pFrameLen );

extern mujoe_err_t mujoe_ser_comm_sendSolicitedDataFrame( MUJOE_SER_COMM ctx, mujoe_sc_sol_data_t *pSolData );

#endif /* MUJOE_SER_COMM_H_ */
