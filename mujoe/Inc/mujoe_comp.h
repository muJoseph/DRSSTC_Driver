/*
 * mujoe_comp.h
 *
 *  Created on: Sep 14, 2025
 *      Author: Joe
 */

#ifndef MUJOE_COMP_H_
#define MUJOE_COMP_H_

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "comp.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"


typedef enum
{
	RSDRV_COMP_HYST_NONE = 0,
	RSDRV_COMP_HYST_10MV,
	RSDRV_COMP_HYST_20MV,
	RSDRV_COMP_HYST_30MV,
	RSDRV_COMP_HYST_40MV,
	RSDRV_COMP_HYST_50MV,
	RSDRV_COMP_HYST_60MV,
	RSDRV_COMP_HYST_70MV,
}mujoe_comp_in_hysteresis_t;

typedef struct _comp_cfg
{

}comp_cfg_t;

typedef struct _comp_ctx
{
	comp_cfg_t  		cfg;
	COMP_TypeDef		*instance;

}comp_ctx_t, *COMP_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_comp_init_context( COMP_CTX ctx, COMP_TypeDef *instance, comp_cfg_t *pCfg );
extern mujoe_err_t mujoe_comp_activate(COMP_CTX ctx);
extern mujoe_err_t mujoe_comp_deactivate(COMP_CTX ctx);

extern uint32_t mujoe_comp_read_output_lvl(COMP_CTX ctx);

extern mujoe_err_t mujoe_comp_set_input_hysteresis( COMP_CTX ctx, mujoe_comp_in_hysteresis_t hyst);


#endif /* MUJOE_COMP_H_ */
