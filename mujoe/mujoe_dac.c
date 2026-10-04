/*
 * mujoe_dac.c
 *
 *      Author: mujoe
 */

////////////////////////////////////////////////////////////////////////
// INCLUDE
////////////////////////////////////////////////////////////////////////

#include "mujoe_dac.h"
#include <string.h>


////////////////////////////////////////////////////////////////////////
// DEFINES
////////////////////////////////////////////////////////////////////////

/* Definitions of environment analog values */
  /* Value of analog reference voltage (Vref+), connected to analog voltage   */
  /* supply Vdda (unit: mV).                                                  */
#define VDDA_APPLI                       ((uint32_t)3300)

/* Definitions of data related to this example */
  /* Full-scale digital value with a resolution of 12 bits (voltage range     */
  /* determined by analog voltage references Vref+ and Vref-,                 */
  /* refer to reference manual).                                              */
#define DIGITAL_SCALE_12BITS             (__LL_DAC_DIGITAL_SCALE(LL_DAC_RESOLUTION_12B))


#define DAC_VREF					3.3f
#define VOLT2DACCODE				4096.0f/DAC_VREF

////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////

mujoe_err_t mujoe_dac_init_context( DAC_CTX ctx, DAC_TypeDef *instance, dac_cfg_t *pCfg )
{
	memset(ctx,0,sizeof(dac_ctx_t));
	// Store ll DAC instance
	ctx->instance = instance;
	ctx->cfg = *pCfg;

	return MUJOE_ERR_NONE;
}// mujoe_dac_init


/**
  * @brief  Perform DAC activation procedure to make it ready to generate
  *         a voltage (DAC instance: DAC1).
  * @note   Operations:
  *         - Enable DAC instance channel
  *         - Wait for DAC instance channel startup time
  * @param  None
  * @retval None
  */
mujoe_err_t mujoe_dac_activate_ch(DAC_CTX ctx, mujoe_dac_ch_t ch)
{

  if( !ctx->cfg.ch1En && ch == MUJOE_DAC_CH1 )
	  return MUJOE_ERR_FAILURE;
  else if( !ctx->cfg.ch2En && ch == MUJOE_DAC_CH2)
	  return MUJOE_ERR_FAILURE;

  __IO uint32_t wait_loop_index = 0;

  /* Enable DAC channel */
  LL_DAC_Enable(ctx->instance, ch);

  /* Delay for DAC channel voltage settling time from DAC channel startup.    */
  /* Compute number of CPU cycles to wait for, from delay in us.              */
  /* Note: Variable divided by 2 to compensate partially                      */
  /*       CPU processing cycles (depends on compilation optimization).       */
  /* Note: If system core clock frequency is below 200kHz, wait time          */
  /*       is only a few CPU processing cycles.                               */
  wait_loop_index = ((LL_DAC_DELAY_STARTUP_VOLTAGE_SETTLING_US * (SystemCoreClock / (100000 * 2))) / 10);
  while(wait_loop_index != 0)
  {
    wait_loop_index--;
  }

  /* Enable DAC channel trigger */
  /* Note: DAC channel conversion can start from trigger enable:              */
  /*       - if DAC channel trigger source is set to SW:                      */
  /*         DAC channel conversion will start after trig order               */
  /*         using function "LL_DAC_TrigSWConversion()".                      */
  /*       - if DAC channel trigger source is set to external trigger         */
  /*         (timer, ...):                                                    */
  /*         DAC channel conversion can start immediately                     */
  /*         (after next trig order from external trigger)                    */
  LL_DAC_EnableTrigger(ctx->instance, ch);

  return LL_DAC_IsEnabled(ctx->instance, ch) == 1 ? MUJOE_ERR_NONE : MUJOE_ERR_FAILURE;
}// mujoe_dac_activate_ch

// Activates both channels
mujoe_err_t mujoe_dac_activate(DAC_CTX ctx)
{
	mujoe_err_t err = MUJOE_ERR_NONE;
	if( ctx->cfg.ch1En)
		err |= mujoe_dac_activate_ch(ctx, MUJOE_DAC_CH1);
	if( ctx->cfg.ch2En)
		err |= mujoe_dac_activate_ch(ctx, MUJOE_DAC_CH2);
	return err;
}// mujoe_dac_activate


void mujoe_dac_write_voltage(DAC_CTX ctx, mujoe_dac_ch_t ch, float voltage_v )
{
	mujoe_dac_write(ctx, ch, voltage_v * VOLT2DACCODE );
}// mujoe_dac_write_voltage

void mujoe_dac_write(DAC_CTX ctx, mujoe_dac_ch_t ch, uint32_t value )
{
	// Clamp input
	if (value > DIGITAL_SCALE_12BITS)
		value = DIGITAL_SCALE_12BITS;

	/* Set the data to be loaded in the data holding register */
	LL_DAC_ConvertData12RightAligned(ctx->instance, ch, value);

	/* Trig DAC conversion by software */
	LL_DAC_TrigSWConversion(ctx->instance, ch);
}// mujoe_dac_write

void mujoe_dac_write_ratio(DAC_CTX ctx, mujoe_dac_ch_t ch, float ratio )
{
	ratio = CLAMP(ratio,0,1.0f);
	uint32_t value = DIGITAL_SCALE_12BITS * ratio;
	value = CLAMP(value,0,DIGITAL_SCALE_12BITS);

	mujoe_dac_write(ctx, ch, value );

}// mujoe_dac_write


