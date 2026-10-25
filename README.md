<figure>
  <img src="emma-mk1-drsstc-readme-banner.jpg" alt="Description for screen readers">
  <figcaption align="center">Figure 1. <i>Emma Mk1</i> DRSSTC</figcaption>
</figure>

<h1>Brief</h1>

STM32CubeIDE project for the ```STM32G474RE``` used in the <i>Emma Mk1 DRSSTC Motherboard</i> PCB.

<h1>Acknowledgements</h1>

Many thanks to [dertrennawieser](https://github.com/dertrennawieser) whose 
[stm32_resonance_driver](https://github.com/dertrennawieser/stm32_resonance_driver) served as the foundation for the <i>DRSSTC_Driver</i>, specifically the ```STM32G474RE``` <i>HRTIM</i> peripheral configuration.


<h1>Emma Mk1 DRSSTC Project Live Documentation</h1>

<strong>[Emma Mk1 DRSSTC](https://www.mujoseph.com/projects/emma-mk1-drsstc)</strong>

<h2>Project-Wide Predefined Symbols</h2>

- Define ```MUJOE_DEBUG_EN``` to enable the ```TRAP_MCU``` macro which will spin forever to assist in debug.
- Define ```RES_DRV_DBG_EN=1``` to enable debug info/error logs and variables. Define ```RES_DRV_DBG_EN=0``` to disable (recommended).
- Define ```PRINTF_ITM_REDIRECT_EN=1``` to route info/error logs to [debugger SWO](https://www.youtube.com/watch?v=rdmJgQW5JrQ&t=275s). Define ```PRINTF_ITM_REDIRECT_EN=0``` to disable.
- Define ```BUZZER_EN``` to enable piezo buzzer drive on ```J3``` of <i>Emma Mk1 DRSSTC Motherboard Rev A</i> PCB.

<h3>Additonal Resources</h3>

- [STM32G474RE Product Page](https://www.st.com/en/microcontrollers-microprocessors/stm32g474re.html)
- [STM32G474RE Datasheet](https://www.st.com/resource/en/datasheet/stm32g474re.pdf)
- [AN4539 HRTIM Cookbook](https://www.st.com/resource/en/application_note/an4539-hrtim-cookbook-stmicroelectronics.pdf)
