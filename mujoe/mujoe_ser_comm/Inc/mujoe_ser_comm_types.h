/*
 * mujoe_ser_comm_types.h
 *
 *  Created on: Oct 13, 2025
 *      Author: Joe
 */

#ifndef MUJOE_SER_COMM_TYPES_H_
#define MUJOE_SER_COMM_TYPES_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_bsp.h"
#include "mujoe_types.h"
#include "mujoe_settings.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////


// Command
typedef uint16_t	sc_cmd_id_t;
#define SC_CMD_ARG_MAX_LEN				MUJOE_SC_MAX_CMD_ARG_LEN
typedef struct _mujoe_sc_cmd
{
	sc_cmd_id_t		cmd_id;
	uint8_t			argLen;
	uint8_t			arg[SC_CMD_ARG_MAX_LEN];
}mujoe_sc_cmd_t;

// Command Response
typedef uint16_t	sc_cmd_rsp_id_t;
typedef uint32_t	sc_flags_t;
#define SC_CMD_RSP_PARAM_MAX_LEN				MUJOE_SC_MAX_CMD_RSP_PARAM_LEN
typedef struct _mujoe_sc_cmd_rsp
{
	sc_cmd_id_t			cmd_id;
	sc_cmd_rsp_id_t		cmd_rsp_id;
	sc_flags_t			flags;
	uint8_t				paramLen;
	uint8_t				params[SC_CMD_RSP_PARAM_MAX_LEN];

}mujoe_sc_cmd_rsp_t;

#define SC_FRAME_ID_CMDRSP_SUCCESS			(sc_cmd_rsp_id_t)0x0000
#define SC_FRAME_ID_CMDRSP_BUSY				(sc_cmd_rsp_id_t)0x0001
#define SC_FRAME_ID_CMDRSP_INV_PARAMS		(sc_cmd_rsp_id_t)0x0002
#define SC_FRAME_ID_CMDRSP_NOT_SUPPORTED	(sc_cmd_rsp_id_t)0x0003
#define SC_FRAME_ID_CMDRSP_FAILURE			(sc_cmd_rsp_id_t)0xFFFF

// Solicited Data
#define SC_SOL_DATA_CHUNK_MAX_LEN				MUJOE_SC_MAX_SOL_DATA_CHUNK_LEN
typedef struct _mujoe_sc_sol_data
{

	uint16_t			data_id;
	uint16_t			data_len;
	uint16_t			chunk_idx;
	uint16_t			chunk_len;
	uint8_t				chunk[SC_SOL_DATA_CHUNK_MAX_LEN];

}mujoe_sc_sol_data_t;

// Frame Payload
typedef union _mujoe_sc_frame_payload
{
	mujoe_sc_cmd_t			mujoe_sc_cmd;
	mujoe_sc_cmd_rsp_t		mujoe_sc_cmd_rsp;
}mujoe_sc_frame_payload_u;

typedef uint16_t	sc_frame_id_t;
typedef uint16_t	sc_frame_payload_len_t;
typedef uint16_t	sc_frame_crc_t;

// Frame IDS
#define SC_FRAME_ID_CMD			(sc_frame_id_t)0x0001	// Command Frame
#define SC_FRAME_ID_CMD_RSP		(sc_frame_id_t)0x0002	// Command Response Frame
#define SC_FRAME_ID_UNSOL		(sc_frame_id_t)0x0003	// Unsolicited Data Frame
#define SC_FRAME_ID_SOL			(sc_frame_id_t)0x0004	// Solicited Data Frame

// Frame
typedef uint32_t preamble_t;
typedef struct _mujoe_sc_frame_hdr
{
	preamble_t					preamble;
	sc_frame_id_t				id;
	sc_frame_payload_len_t		payloadLen;

}mujoe_sc_frame_hdr_t;

typedef struct _mujoe_sc_frame
{
	mujoe_sc_frame_hdr_t		hdr;
	mujoe_sc_frame_payload_u	payload;
	sc_frame_crc_t				crc;

}mujoe_sc_frame_t;

#define MUJOE_SC_FRAME_BASE_LEN		sizeof(preamble_t) + sizeof(sc_frame_id_t) + sizeof(sc_frame_payload_len_t) + sizeof(sc_frame_crc_t)

#define MUJOE_SC_FRAME_PREAMBLE		(preamble_t)0xDEADBEEF

typedef  bool (*mujoe_sc_cmd_handler_Fp_t)( void *uArg, uint8_t *pCmdArg, int cmdArgLen, mujoe_sc_cmd_rsp_t *pCmdRsp );
typedef struct _mujoe_sc_cmd_handler
{
	sc_cmd_id_t 				cmdId;
	mujoe_sc_cmd_handler_Fp_t	handlerCb;

}mujoe_sc_cmd_handler_t;

#endif /* MUJOE_SER_COMM_TYPES_H_ */
