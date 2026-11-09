#include "can_if.h"
#include <string.h>
#include "stm32u5xx_hal.h"

typedef struct { uint32_t bps; uint16_t presc; uint8_t seg1, seg2, sjw; } bit_timing_t;

static const bit_timing_t bt_table[] = {
  {   10000u, 500u, 12u, 3u, 3u },
  {   20000u, 250u, 12u, 3u, 3u },
  {   50000u, 100u, 12u, 3u, 3u },
  {  100000u,  50u, 12u, 3u, 3u },
  {  125000u,  40u, 12u, 3u, 3u },
  {  250000u,  20u, 12u, 3u, 3u },
  {  500000u,  10u, 12u, 3u, 3u },
  {  800000u,   5u, 15u, 4u, 4u },
  { 1000000u,   5u, 12u, 3u, 3u },
};

#define RX_RING_SIZE  256u
#define TX_QUEUE_SIZE 64u

static FDCAN_HandleTypeDef  hfdcan;
static const bit_timing_t  *bt = &bt_table[6];
static can_mode_t           mode = CAN_CLOSED;
static bool                 in_busoff;

static can_frame_t          rx_ring[RX_RING_SIZE];
static volatile uint32_t    rx_head, rx_tail;
static can_frame_t          tx_queue[TX_QUEUE_SIZE];
static uint32_t             tx_head, tx_tail;

static volatile can_stats_t stats;


void can_park_pins(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);

  GPIO_InitTypeDef g = {0};
  g.Pin   = GPIO_PIN_12;
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &g);

  g.Pin  = GPIO_PIN_11;
  g.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOA, &g);
}

bool can_set_bitrate(uint32_t bps)
{
  if (mode != CAN_CLOSED)
    return false;
  for (uint32_t i = 0; i < sizeof(bt_table) / sizeof(bt_table[0]); i++)
  {
    if (bt_table[i].bps == bps)
    {
      bt = &bt_table[i];
      return true;
    }
  }
  return false;
}

uint32_t can_get_bitrate(void) { return bt->bps; }
can_mode_t can_mode(void)      { return mode; }

bool can_open(bool listen_only)
{
  if (mode != CAN_CLOSED)
    can_close();

  rx_head = rx_tail = 0;
  tx_head = tx_tail = 0;
  in_busoff = false;

  hfdcan.Instance                  = FDCAN1;
  hfdcan.Init.ClockDivider         = FDCAN_CLOCK_DIV1;
  hfdcan.Init.FrameFormat          = FDCAN_FRAME_CLASSIC;
  hfdcan.Init.Mode                 = listen_only ? FDCAN_MODE_BUS_MONITORING : FDCAN_MODE_NORMAL;
  hfdcan.Init.AutoRetransmission   = ENABLE;
  hfdcan.Init.TransmitPause        = DISABLE;
  hfdcan.Init.ProtocolException    = DISABLE;
  hfdcan.Init.NominalPrescaler     = bt->presc;
  hfdcan.Init.NominalSyncJumpWidth = bt->sjw;
  hfdcan.Init.NominalTimeSeg1      = bt->seg1;
  hfdcan.Init.NominalTimeSeg2      = bt->seg2;
  hfdcan.Init.DataPrescaler        = 1;
  hfdcan.Init.DataSyncJumpWidth    = 1;
  hfdcan.Init.DataTimeSeg1         = 1;
  hfdcan.Init.DataTimeSeg2         = 1;
  hfdcan.Init.StdFiltersNbr        = 0;
  hfdcan.Init.ExtFiltersNbr        = 0;
  hfdcan.Init.TxFifoQueueMode      = FDCAN_TX_FIFO_OPERATION;

  if (HAL_FDCAN_Init(&hfdcan) != HAL_OK)
    return false;

  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan, FDCAN_ACCEPT_IN_RX_FIFO0, FDCAN_ACCEPT_IN_RX_FIFO0,
                                   FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
    return false;

  if (HAL_FDCAN_ActivateNotification(&hfdcan,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST, 0) != HAL_OK)
    return false;

  if (HAL_FDCAN_Start(&hfdcan) != HAL_OK)
    return false;

  mode = listen_only ? CAN_OPEN_LISTEN : CAN_OPEN_NORMAL;
  return true;
}

void can_close(void)
{
  if (mode == CAN_CLOSED)
    return;
  (void)HAL_FDCAN_Stop(&hfdcan);
  (void)HAL_FDCAN_DeInit(&hfdcan);
  mode = CAN_CLOSED;
  rx_head = rx_tail = 0;
  tx_head = tx_tail = 0;
  in_busoff = false;
}


bool can_send(const can_frame_t *f)
{
  uint32_t next = (tx_head + 1u) & (TX_QUEUE_SIZE - 1u);
  if (mode != CAN_OPEN_NORMAL || next == tx_tail)
  {
    stats.tx_drop++;
    return false;
  }
  tx_queue[tx_head] = *f;
  tx_head = next;
  return true;
}

bool can_peek(can_frame_t *f)
{
  if (rx_tail == rx_head)
    return false;
  __DMB();
  *f = rx_ring[rx_tail];
  return true;
}

void can_pop(void)
{
  if (rx_tail != rx_head)
    rx_tail = (rx_tail + 1u) & (RX_RING_SIZE - 1u);
}

void can_poll(void)
{
  if (mode == CAN_CLOSED)
    return;

  if (FDCAN1->PSR & FDCAN_PSR_BO)
  {
    if (!in_busoff)
    {
      in_busoff = true;
      stats.busoff++;
    }
    if (FDCAN1->CCCR & FDCAN_CCCR_INIT)
      CLEAR_BIT(FDCAN1->CCCR, FDCAN_CCCR_INIT);
    return;
  }
  in_busoff = false;

  while (tx_tail != tx_head && HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan) > 0u)
  {
    const can_frame_t *f = &tx_queue[tx_tail];
    FDCAN_TxHeaderTypeDef h = {
      .Identifier          = f->id,
      .IdType              = f->ext ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID,
      .TxFrameType         = f->rtr ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME,
      .DataLength          = f->len,
      .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
      .BitRateSwitch       = FDCAN_BRS_OFF,
      .FDFormat            = FDCAN_CLASSIC_CAN,
      .TxEventFifoControl  = FDCAN_NO_TX_EVENTS,
      .MessageMarker       = 0,
    };
    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &h, f->data) != HAL_OK)
      break;
    stats.tx++;
    tx_tail = (tx_tail + 1u) & (TX_QUEUE_SIZE - 1u);
  }
}


void can_get_stats(can_stats_t *s)
{
  s->rx         = stats.rx;
  s->tx         = stats.tx;
  s->rx_lost_hw = stats.rx_lost_hw;
  s->rx_lost_sw = stats.rx_lost_sw;
  s->tx_drop    = stats.tx_drop;
  s->busoff     = stats.busoff;
}

void can_get_errors(uint8_t *tec, uint8_t *rec, bool *passive, bool *busoff)
{
  if (mode == CAN_CLOSED)
  {
    *tec = *rec = 0;
    *passive = *busoff = false;
    return;
  }
  uint32_t ecr = FDCAN1->ECR;
  uint32_t psr = FDCAN1->PSR;
  *tec     = (uint8_t)((ecr & FDCAN_ECR_TEC_Msk) >> FDCAN_ECR_TEC_Pos);
  *rec     = (uint8_t)((ecr & FDCAN_ECR_REC_Msk) >> FDCAN_ECR_REC_Pos);
  *passive = (psr & FDCAN_PSR_EP) != 0u;
  *busoff  = (psr & FDCAN_PSR_BO) != 0u;
}


void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *h, uint32_t its)
{
  if (its & FDCAN_IT_RX_FIFO0_MESSAGE_LOST)
    stats.rx_lost_hw++;

  FDCAN_RxHeaderTypeDef hdr;
  uint8_t d[8];
  while (HAL_FDCAN_GetRxFifoFillLevel(h, FDCAN_RX_FIFO0) > 0u)
  {
    if (HAL_FDCAN_GetRxMessage(h, FDCAN_RX_FIFO0, &hdr, d) != HAL_OK)
      break;

    uint32_t head = rx_head;
    uint32_t next = (head + 1u) & (RX_RING_SIZE - 1u);
    if (next == rx_tail)
    {
      stats.rx_lost_sw++;
      continue;
    }

    can_frame_t *f = &rx_ring[head];
    f->id  = hdr.Identifier;
    f->ext = (hdr.IdType == FDCAN_EXTENDED_ID);
    f->rtr = (hdr.RxFrameType == FDCAN_REMOTE_FRAME);
    f->len = (hdr.DataLength > 8u) ? 8u : (uint8_t)hdr.DataLength;
    if (f->rtr)
      memset(f->data, 0, 8);
    else
      memcpy(f->data, d, 8);

    __DMB();
    rx_head = next;
    stats.rx++;
  }
}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *h)
{
  if (h->Instance != FDCAN1)
    return;

  __HAL_RCC_FDCAN1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef g = {0};
  g.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
  g.Mode      = GPIO_MODE_AF_PP;
  g.Pull      = GPIO_NOPULL;
  g.Speed     = GPIO_SPEED_FREQ_LOW;
  g.Alternate = GPIO_AF9_FDCAN1;
  HAL_GPIO_Init(GPIOA, &g);

  HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
}

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *h)
{
  if (h->Instance != FDCAN1)
    return;
  HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
  __HAL_RCC_FDCAN1_FORCE_RESET();
  __HAL_RCC_FDCAN1_RELEASE_RESET();
  __HAL_RCC_FDCAN1_CLK_DISABLE();
  can_park_pins();
}

void FDCAN1_IT0_IRQHandler(void)
{
  HAL_FDCAN_IRQHandler(&hfdcan);
}
