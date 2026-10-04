/*
 * mujoe_dac.h
 *
 *      Author: mujoe
 */

#ifndef __MUJOE_DAC_H__
#define __MUJOE_DAC_H__

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "dac.h"
#include "mujoe_bsp.h"
#include "mujoe_types.h"

////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////

typedef enum
{
	MUJOE_DAC_CH1 = LL_DAC_CHANNEL_1,
	MUJOE_DAC_CH2 = LL_DAC_CHANNEL_2,

}mujoe_dac_ch_t;

typedef struct _dac_cfg
{
	bool ch1En;
	bool ch2En;
}dac_cfg_t;

typedef struct _dac_ctx
{
	dac_cfg_t  		cfg;
	DAC_TypeDef		*instance;

}dac_ctx_t, *DAC_CTX;

////////////////////////////////////////////////////////////////////////
// API
////////////////////////////////////////////////////////////////////////

extern mujoe_err_t mujoe_dac_init_context( DAC_CTX ctx, DAC_TypeDef *instance, dac_cfg_t *pCfg );
extern mujoe_err_t mujoe_dac_activate_ch(DAC_CTX ctx, mujoe_dac_ch_t ch);
extern mujoe_err_t mujoe_dac_activate(DAC_CTX ctx);

extern void mujoe_dac_write(DAC_CTX ctx, mujoe_dac_ch_t ch, uint32_t value );
extern void mujoe_dac_write_ratio(DAC_CTX ctx, mujoe_dac_ch_t ch, float ratio );
extern void mujoe_dac_write_voltage(DAC_CTX ctx, mujoe_dac_ch_t ch, float voltage_v );


#endif // #ifndef __MUJOE_DAC_H__
