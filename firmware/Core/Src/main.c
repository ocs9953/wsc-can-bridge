#include "stm32u5xx_hal.h"
#include "can_if.h"
#include "rtt.h"
#include "slcan.h"

#define LED_ERR_PORT    GPIOE
#define LED_ERR_PIN     GPIO_PIN_11
#define LED_BOARD_PORT  GPIOE
#define LED_BOARD_PIN   GPIO_PIN_12
#define LED_CAN_PORT    GPIOB
#define LED_CAN_PIN     GPIO_PIN_15

static void SystemPower_Config(void);
static void SystemClock_Config(void);
static void LED_Init(void);
static void LED_Update(void);
void Error_Handler(void);

int main(void)
{
  HAL_Init();
  SystemPower_Config();
  SystemClock_Config();
  if (HAL_ICACHE_Enable() != HAL_OK)
    Error_Handler();

  can_park_pins();
  LED_Init();
  rtt_init();
  slcan_init();

  for (;;)
  {
    slcan_poll();
    LED_Update();
  }
}


static void LED_Init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN | LED_BOARD_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_CAN_PORT, LED_CAN_PIN, GPIO_PIN_RESET);

  GPIO_InitTypeDef g = {0};
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  g.Pin   = LED_ERR_PIN | LED_BOARD_PIN;
  HAL_GPIO_Init(GPIOE, &g);
  g.Pin   = LED_CAN_PIN;
  HAL_GPIO_Init(GPIOB, &g);
}

static void LED_Update(void)
{
  static uint32_t last_tick, last_traffic, last_lost, err_until;
  uint32_t now = HAL_GetTick();
  if (now - last_tick < 50u)
    return;
  last_tick = now;

  can_stats_t s;
  uint8_t tec, rec;
  bool ep, bo;
  can_get_stats(&s);
  can_get_errors(&tec, &rec, &ep, &bo);

  bool open = (can_mode() != CAN_CLOSED);
  HAL_GPIO_WritePin(LED_BOARD_PORT, LED_BOARD_PIN,
                    (open || (now % 1000u) < 500u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

  uint32_t traffic = s.rx + s.tx;
  HAL_GPIO_WritePin(LED_CAN_PORT, LED_CAN_PIN,
                    (traffic != last_traffic) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  last_traffic = traffic;

  uint32_t lost = s.rx_lost_hw + s.rx_lost_sw + s.busoff;
  if (lost != last_lost || ep || bo)
    err_until = now + 1000u;
  last_lost = lost;
  HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN,
                    ((int32_t)(err_until - now) > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    Error_Handler();

  osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState            = RCC_HSE_ON;
  osc.PLL.PLLState        = RCC_PLL_ON;
  osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMBOOST       = RCC_PLLMBOOST_DIV2;
  osc.PLL.PLLM            = 1;
  osc.PLL.PLLN            = 10;
  osc.PLL.PLLP            = 2;
  osc.PLL.PLLQ            = 2;
  osc.PLL.PLLR            = 1;
  osc.PLL.PLLRGE          = RCC_PLLVCIRANGE_1;
  osc.PLL.PLLFRACN        = 0;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    Error_Handler();

  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  clk.APB3CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4) != HAL_OK)
    Error_Handler();

  RCC_PeriphCLKInitTypeDef pclk = {0};
  pclk.PeriphClockSelection = RCC_PERIPHCLK_FDCAN1;
  pclk.Fdcan1ClockSelection = RCC_FDCAN1CLKSOURCE_PLL1;
  if (HAL_RCCEx_PeriphCLKConfig(&pclk) != HAL_OK)
    Error_Handler();
}

void HAL_MspInit(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
}

static void SystemPower_Config(void)
{
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
    Error_Handler();
}

void Error_Handler(void)
{
  __disable_irq();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  GPIO_InitTypeDef g = { .Pin = LED_ERR_PIN, .Mode = GPIO_MODE_OUTPUT_PP };
  HAL_GPIO_Init(LED_ERR_PORT, &g);
  HAL_GPIO_WritePin(LED_ERR_PORT, LED_ERR_PIN, GPIO_PIN_SET);
  for (;;) { }
}
