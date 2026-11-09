#include "stm32u5xx_hal.h"

void Error_Handler(void);

void SysTick_Handler(void)
{
  HAL_IncTick();
}

void HardFault_Handler(void)  { Error_Handler(); }
void MemManage_Handler(void)  { Error_Handler(); }
void BusFault_Handler(void)   { Error_Handler(); }
void UsageFault_Handler(void) { Error_Handler(); }
