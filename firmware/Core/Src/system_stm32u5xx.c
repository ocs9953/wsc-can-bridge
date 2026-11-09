/**
  ******************************************************************************
  * @file    system_stm32u5xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M33 Device Peripheral Access Layer System Source File
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32u5xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *   After each device reset the MSI (4 MHz) is used as system clock source.
  *   Then SystemInit() function is called, in "startup_stm32u5xx.s" file, to
  *   configure the system clock before to branch to main program.
  *
  *   This file configures the system clock as follows:
  *=============================================================================
  *-----------------------------------------------------------------------------
  *        System Clock source                     | MSI
  *-----------------------------------------------------------------------------
  *        SYSCLK(Hz)                              | 4000000
  *-----------------------------------------------------------------------------
  *        HCLK(Hz)                                | 4000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                           | 1
  *-----------------------------------------------------------------------------
  *        APB1 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB2 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB3 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        PLL1_SRC                                | No clock
  *-----------------------------------------------------------------------------
  *        PLL1_M                                  | 1
  *-----------------------------------------------------------------------------
  *        PLL1_N                                  | 8
  *-----------------------------------------------------------------------------
  *        PLL1_P                                  | 7
  *-----------------------------------------------------------------------------
  *        PLL1_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL1_R                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL2_SRC                                | NA
  *-----------------------------------------------------------------------------
  *        PLL2_M                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL2_N                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL2_P                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL2_Q                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL2_R                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL3_SRC                                | NA
  *-----------------------------------------------------------------------------
  *        PLL3_M                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL3_N                                  | NA
  *-----------------------------------------------------------------------------
  *        PLL3_P                                  | NA
  *-----------------------------------------------------------------------------
  *        Require 48MHz for USB FS,               | Disabled
  *        SDIO and RNG clock                      |
  *-----------------------------------------------------------------------------
  *=============================================================================
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


#include "stm32u5xx.h"
#include <math.h>


#if !defined  (HSE_VALUE)
  #define HSE_VALUE    16000000U
#endif

#if !defined  (MSI_VALUE)
  #define MSI_VALUE    4000000U
#endif

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    16000000U
#endif

#if !defined(VECT_TAB_OFFSET)
#define VECT_TAB_OFFSET  0x00000000UL
#endif


  uint32_t SystemCoreClock = 4000000U;

  const uint8_t  AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
  const uint8_t  APBPrescTable[8] =  {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
  const uint32_t MSIRangeTable[16] = {48000000U,24000000U,16000000U,12000000U, 4000000U, 2000000U, 1330000U,\
                                      1000000U, 3072000U, 1536000U,1024000U, 768000U, 400000U, 200000U, 133000U, 100000U};


void SystemInit(void)
{
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
   SCB->CPACR |= ((3UL << 20U)|(3UL << 22U));
  #endif

  RCC->CR = RCC_CR_MSISON;

  RCC->CFGR1 = 0U;
  RCC->CFGR2 = 0U;
  RCC->CFGR3 = 0U;

  RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);

  RCC->PLL1CFGR = 0U;

  RCC->CR &= ~(RCC_CR_HSEBYP);

  RCC->CIER = 0U;

  #ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM1_BASE | VECT_TAB_OFFSET;
  #else
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
  #endif
}

void SystemCoreClockUpdate(void)
{
  uint32_t pllr, pllsource, pllm , tmp, pllfracen, msirange;
  float_t fracn1, pllvco;

  if(READ_BIT(RCC->ICSCR1, RCC_ICSCR1_MSIRGSEL) == 0U)
  {
    msirange = (RCC->CSR & RCC_CSR_MSISSRANGE) >> RCC_CSR_MSISSRANGE_Pos;
  }
  else
  {
    msirange = (RCC->ICSCR1 & RCC_ICSCR1_MSISRANGE) >> RCC_ICSCR1_MSISRANGE_Pos;
  }

  msirange = MSIRangeTable[msirange];

  switch (RCC->CFGR1 & RCC_CFGR1_SWS)
  {
  case 0x00:
    SystemCoreClock = msirange;
    break;

  case 0x04:
    SystemCoreClock = HSI_VALUE;
    break;

  case 0x08:
    SystemCoreClock = HSE_VALUE;
    break;

  case 0x0C:
    pllsource = (RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC);
    pllm = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M)>> RCC_PLL1CFGR_PLL1M_Pos) + 1U;
    pllfracen = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1FRACEN)>>RCC_PLL1CFGR_PLL1FRACEN_Pos);
    fracn1 = (float_t)(uint32_t)(pllfracen* ((RCC->PLL1FRACR & RCC_PLL1FRACR_PLL1FRACN)>> RCC_PLL1FRACR_PLL1FRACN_Pos));

      switch (pllsource)
      {
      case 0x00:
        pllvco = (float_t)0U;
        break;

      case 0x02:
        pllvco = ((float_t)HSI_VALUE / (float_t)pllm);
        break;

      case 0x03:
        pllvco = ((float_t)HSE_VALUE / (float_t)pllm);
        break;

      default:
        pllvco = ((float_t)msirange / (float_t)pllm);
        break;
      }

      pllvco = pllvco * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + (fracn1/(float_t)0x2000) + (float_t)1U);
      pllr = (((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1R) >> RCC_PLL1DIVR_PLL1R_Pos) + 1U );
      SystemCoreClock = (uint32_t)((uint32_t)pllvco/pllr);
      break;

  default:
    SystemCoreClock = msirange;
    break;
  }
  tmp = AHBPrescTable[((RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos)];
  SystemCoreClock >>= tmp;
}


