/**
  ******************************************************************************
  * @file    stm32u5xx_hal_conf.h
  * @author  MCD Application Team
  * @brief   HAL configuration file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021-2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef STM32U5xx_HAL_CONF_H
#define STM32U5xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif


#define HAL_MODULE_ENABLED

#define HAL_FDCAN_MODULE_ENABLED
#define HAL_ICACHE_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

#if !defined  (HSE_VALUE)
#define HSE_VALUE              16000000UL
#endif

#if !defined  (HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100UL
#endif

#if !defined  (MSI_VALUE)
#define MSI_VALUE              4000000UL
#endif

#if !defined  (HSI_VALUE)
#define HSI_VALUE              16000000UL
#endif

#if !defined  (HSI48_VALUE)
#define HSI48_VALUE             48000000UL
#endif

#if !defined  (LSI_VALUE)
#define LSI_VALUE               32000UL
#endif

#if !defined  (LSE_VALUE)
#define LSE_VALUE              32768UL
#endif

#if !defined  (LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    5000UL
#endif

#if !defined  (EXTERNAL_SAI1_CLOCK_VALUE)
#define EXTERNAL_SAI1_CLOCK_VALUE  48000UL
#endif


#define  VDD_VALUE                    3300UL
#define  TICK_INT_PRIORITY            (14UL)

#define  USE_RTOS                     0U
#define  PREFETCH_ENABLE              1U


#define  USE_HAL_ADC_REGISTER_CALLBACKS        0U
#define  USE_HAL_COMP_REGISTER_CALLBACKS       0U
#define  USE_HAL_CORDIC_REGISTER_CALLBACKS     0U
#define  USE_HAL_CRYP_REGISTER_CALLBACKS       0U
#define  USE_HAL_DAC_REGISTER_CALLBACKS        0U
#define  USE_HAL_DCMI_REGISTER_CALLBACKS       0U
#define  USE_HAL_DMA2D_REGISTER_CALLBACKS      0U
#define  USE_HAL_DSI_REGISTER_CALLBACKS        0U
#define  USE_HAL_FDCAN_REGISTER_CALLBACKS      0U
#define  USE_HAL_FMAC_REGISTER_CALLBACKS       0U
#define  USE_HAL_HASH_REGISTER_CALLBACKS       0U
#define  USE_HAL_HCD_REGISTER_CALLBACKS        0U
#define  USE_HAL_GFXMMU_REGISTER_CALLBACKS     0U
#define  USE_HAL_GFXTIM_REGISTER_CALLBACKS     0U
#define  USE_HAL_GPU2D_REGISTER_CALLBACKS      0U
#define  USE_HAL_I2C_REGISTER_CALLBACKS        0U
#define  USE_HAL_IWDG_REGISTER_CALLBACKS       0U
#define  USE_HAL_IRDA_REGISTER_CALLBACKS       0U
#define  USE_HAL_JPEG_REGISTER_CALLBACKS       0U
#define  USE_HAL_LPTIM_REGISTER_CALLBACKS      0U
#define  USE_HAL_LTDC_REGISTER_CALLBACKS       0U
#define  USE_HAL_MDF_REGISTER_CALLBACKS        0U
#define  USE_HAL_MMC_REGISTER_CALLBACKS        0U
#define  USE_HAL_NAND_REGISTER_CALLBACKS       0U
#define  USE_HAL_NOR_REGISTER_CALLBACKS        0U
#define  USE_HAL_OPAMP_REGISTER_CALLBACKS      0U
#define  USE_HAL_OTFDEC_REGISTER_CALLBACKS     0U
#define  USE_HAL_PCD_REGISTER_CALLBACKS        0U
#define  USE_HAL_PKA_REGISTER_CALLBACKS        0U
#define  USE_HAL_RAMCFG_REGISTER_CALLBACKS     0U
#define  USE_HAL_RNG_REGISTER_CALLBACKS        0U
#define  USE_HAL_RTC_REGISTER_CALLBACKS        0U
#define  USE_HAL_SAI_REGISTER_CALLBACKS        0U
#define  USE_HAL_SD_REGISTER_CALLBACKS         0U
#define  USE_HAL_SDIO_REGISTER_CALLBACKS       0U
#define  USE_HAL_SDRAM_REGISTER_CALLBACKS      0U
#define  USE_HAL_SMARTCARD_REGISTER_CALLBACKS  0U
#define  USE_HAL_SMBUS_REGISTER_CALLBACKS      0U
#define  USE_HAL_SPI_REGISTER_CALLBACKS        0U
#define  USE_HAL_SRAM_REGISTER_CALLBACKS       0U
#define  USE_HAL_TIM_REGISTER_CALLBACKS        0U
#define  USE_HAL_TSC_REGISTER_CALLBACKS        0U
#define  USE_HAL_UART_REGISTER_CALLBACKS       0U
#define  USE_HAL_USART_REGISTER_CALLBACKS      0U
#define  USE_HAL_WWDG_REGISTER_CALLBACKS       0U
#define  USE_HAL_OSPI_REGISTER_CALLBACKS       0U

#define USE_SPI_CRC                   0U


#define USE_SD_TRANSCEIVER            0U

#define SDMMC_SWDATATIMEOUT           1000U

#define USE_SDIO_TRANSCEIVER          0U
#define SDIO_MAX_IO_NUMBER            7U


#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32u5xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32u5xx_hal_gpio.h"
#endif

#ifdef HAL_ICACHE_MODULE_ENABLED
#include "stm32u5xx_hal_icache.h"
#endif

#ifdef HAL_DCACHE_MODULE_ENABLED
#include "stm32u5xx_hal_dcache.h"
#endif

#ifdef HAL_GTZC_MODULE_ENABLED
#include "stm32u5xx_hal_gtzc.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32u5xx_hal_dma.h"
#endif

#ifdef HAL_DMA2D_MODULE_ENABLED
#include "stm32u5xx_hal_dma2d.h"
#endif

#ifdef HAL_DSI_MODULE_ENABLED
#include "stm32u5xx_hal_dsi.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32u5xx_hal_cortex.h"
#endif

#ifdef HAL_PKA_MODULE_ENABLED
#include "stm32u5xx_hal_pka.h"
#endif

#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32u5xx_hal_adc.h"
#endif

#ifdef HAL_COMP_MODULE_ENABLED
#include "stm32u5xx_hal_comp.h"
#endif

#ifdef HAL_CRC_MODULE_ENABLED
#include "stm32u5xx_hal_crc.h"
#endif

#ifdef HAL_CRYP_MODULE_ENABLED
#include "stm32u5xx_hal_cryp.h"
#endif

#ifdef HAL_DAC_MODULE_ENABLED
#include "stm32u5xx_hal_dac.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32u5xx_hal_flash.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
#include "stm32u5xx_hal_hash.h"
#endif

#ifdef HAL_SRAM_MODULE_ENABLED
#include "stm32u5xx_hal_sram.h"
#endif

#ifdef HAL_MMC_MODULE_ENABLED
#include "stm32u5xx_hal_mmc.h"
#endif

#ifdef HAL_NOR_MODULE_ENABLED
#include "stm32u5xx_hal_nor.h"
#endif

#ifdef HAL_NAND_MODULE_ENABLED
#include "stm32u5xx_hal_nand.h"
#endif

#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32u5xx_hal_i2c.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32u5xx_hal_iwdg.h"
#endif

#ifdef HAL_JPEG_MODULE_ENABLED
#include "stm32u5xx_hal_jpeg.h"
#endif

#ifdef HAL_LPTIM_MODULE_ENABLED
#include "stm32u5xx_hal_lptim.h"
#endif

#ifdef HAL_LTDC_MODULE_ENABLED
#include "stm32u5xx_hal_ltdc.h"
#endif

#ifdef HAL_OPAMP_MODULE_ENABLED
#include "stm32u5xx_hal_opamp.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32u5xx_hal_pwr.h"
#endif

#ifdef HAL_OSPI_MODULE_ENABLED
#include "stm32u5xx_hal_ospi.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
#include "stm32u5xx_hal_rng.h"
#endif

#ifdef HAL_RTC_MODULE_ENABLED
#include "stm32u5xx_hal_rtc.h"
#endif

#ifdef HAL_SAI_MODULE_ENABLED
#include "stm32u5xx_hal_sai.h"
#endif

#ifdef HAL_SD_MODULE_ENABLED
#include "stm32u5xx_hal_sd.h"
#endif

#ifdef HAL_SDIO_MODULE_ENABLED
#include "stm32u5xx_hal_sdio.h"
#endif

#ifdef HAL_SMBUS_MODULE_ENABLED
#include "stm32u5xx_hal_smbus.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
#include "stm32u5xx_hal_spi.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32u5xx_hal_tim.h"
#endif

#ifdef HAL_TSC_MODULE_ENABLED
#include "stm32u5xx_hal_tsc.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
#include "stm32u5xx_hal_uart.h"
#endif

#ifdef HAL_USART_MODULE_ENABLED
#include "stm32u5xx_hal_usart.h"
#endif

#ifdef HAL_IRDA_MODULE_ENABLED
#include "stm32u5xx_hal_irda.h"
#endif

#ifdef HAL_SMARTCARD_MODULE_ENABLED
#include "stm32u5xx_hal_smartcard.h"
#endif

#ifdef HAL_WWDG_MODULE_ENABLED
#include "stm32u5xx_hal_wwdg.h"
#endif

#ifdef HAL_PCD_MODULE_ENABLED
#include "stm32u5xx_hal_pcd.h"
#endif

#ifdef HAL_HCD_MODULE_ENABLED
#include "stm32u5xx_hal_hcd.h"
#endif

#ifdef HAL_CORDIC_MODULE_ENABLED
#include "stm32u5xx_hal_cordic.h"
#endif

#ifdef HAL_DCMI_MODULE_ENABLED
#include "stm32u5xx_hal_dcmi.h"
#endif

#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32u5xx_hal_exti.h"
#endif

#ifdef HAL_FDCAN_MODULE_ENABLED
#include "stm32u5xx_hal_fdcan.h"
#endif

#ifdef HAL_FMAC_MODULE_ENABLED
#include "stm32u5xx_hal_fmac.h"
#endif

#ifdef HAL_GFXMMU_MODULE_ENABLED
#include "stm32u5xx_hal_gfxmmu.h"
#endif

#ifdef HAL_GFXTIM_MODULE_ENABLED
#include "stm32u5xx_hal_gfxtim.h"
#endif

#ifdef HAL_GPU2D_MODULE_ENABLED
#include "stm32u5xx_hal_gpu2d.h"
#endif

#ifdef HAL_OTFDEC_MODULE_ENABLED
#include "stm32u5xx_hal_otfdec.h"
#endif

#ifdef HAL_PSSI_MODULE_ENABLED
#include "stm32u5xx_hal_pssi.h"
#endif

#ifdef HAL_RAMCFG_MODULE_ENABLED
#include "stm32u5xx_hal_ramcfg.h"
#endif

#ifdef HAL_MDF_MODULE_ENABLED
#include "stm32u5xx_hal_mdf.h"
#endif

#ifdef HAL_XSPI_MODULE_ENABLED
#include "stm32u5xx_hal_xspi.h"
#endif

#ifdef  USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif

