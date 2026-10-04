/*
 * mujoe_gptim.h
 *
 *  Created on: Sep 17, 2025
 *      Author: Joe
 */

#ifndef MUJOE_GPTIM_H_
#define MUJOE_GPTIM_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "tim.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////


typedef enum
{
	MUJOE_GPTIM2 = (uint8_t)0,
	MUJOE_GPTIM3,
	MUJOE_GPTIM4,
	MUJOE_GPTIM5,
	NUM_GP_TIMERS,

}mujoe_gptim_id_t;


typedef enum
{
	GPTIM_CFG_PRESET_PWM_GEN_DMA = (uint8_t)0,
	GPTIM_CFG_PRESET_SLAVE_RST_W_TRGO_GEN,
}gptim_cfg_preset_id_t;

typedef struct _gptim_cfg
{
	mujoe_gptim_id_t			id;
	gptim_cfg_preset_id_t		preset_id;
	bool						interruptsEnabled;

}gptim_cfg_t;

typedef struct _gptim_ctx
{
	gptim_cfg_t  		cfg;
	TIM_TypeDef			*instance;

}gptim_ctx_t, *GPTIM_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_gptim_init_context( GPTIM_CTX ctx, gptim_cfg_t *pCfg );
extern mujoe_err_t mujoe_gptim_init_hardware( GPTIM_CTX ctx );
extern mujoe_err_t mujoe_gptim_start( GPTIM_CTX ctx, uint32_t Channels );
extern mujoe_err_t mujoe_gptim_startTimer( GPTIM_CTX ctx);
extern mujoe_err_t mujoe_gptim_stop( GPTIM_CTX ctx);


extern void mujoe_gptim_setCounter( GPTIM_CTX ctx, uint32_t counter );
extern mujoe_err_t mujoe_gptim_restart( GPTIM_CTX ctx );

extern mujoe_err_t mujoe_gptim_channel_control( GPTIM_CTX ctx, uint32_t Channels, bool enable );
extern mujoe_err_t mujoe_gptim_controlAllOutputs( GPTIM_CTX ctx, bool enable );

extern void mujoe_gptim_generate_update_event( GPTIM_CTX ctx );

extern mujoe_err_t mujoe_gptim_set_update_evt_period( GPTIM_CTX ctx, uint32_t period_usec );

extern mujoe_err_t mujoe_gptim_set_compare1_period( GPTIM_CTX ctx, uint32_t period_nsec );

extern mujoe_err_t mujoe_gptim_set_compareX_value( GPTIM_CTX ctx,
													uint8_t cmpCh,
													uint16_t cmpVal );

extern mujoe_err_t mujoe_gptim_set_ARR( GPTIM_CTX ctx, uint32_t arr );
extern mujoe_err_t mujoe_gptim_set_prescaler( GPTIM_CTX ctx, uint32_t psc );
extern mujoe_err_t mujoe_gptim_ctrl_arrPreload( GPTIM_CTX ctx, bool enable );

#endif /* MUJOE_GPTIM_H_ */
