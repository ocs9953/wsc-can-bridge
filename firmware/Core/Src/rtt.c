#include "rtt.h"
#include <string.h>
#include "stm32u5xx.h"

#define RTT_UP_SIZE    16384u
#define RTT_DOWN_SIZE  4096u

typedef struct {
  const char        *name;
  char              *buf;
  uint32_t           size;
  volatile uint32_t  wr;
  volatile uint32_t  rd;
  uint32_t           flags;
} rtt_ring_t;

typedef struct {
  char        id[16];
  int32_t     max_up;
  int32_t     max_down;
  rtt_ring_t  up[1];
  rtt_ring_t  down[1];
} rtt_cb_t;

__attribute__((section(".rtt_cb"), used)) rtt_cb_t _SEGGER_RTT;

static char up_buf[RTT_UP_SIZE];
static char down_buf[RTT_DOWN_SIZE];

void rtt_init(void)
{
  static const char id[] = "SEGGER RTT";
  rtt_cb_t *cb = &_SEGGER_RTT;

  memset((void *)cb, 0, sizeof(*cb));
  cb->max_up   = 1;
  cb->max_down = 1;
  cb->up[0]   = (rtt_ring_t){ "Terminal", up_buf,   RTT_UP_SIZE,   0, 0, 0 };
  cb->down[0] = (rtt_ring_t){ "Terminal", down_buf, RTT_DOWN_SIZE, 0, 0, 0 };

  __DMB();
  for (uint32_t i = 0; i < sizeof(id); i++)
    cb->id[i] = id[i];
  __DMB();
}

uint32_t rtt_write_free(void)
{
  const rtt_ring_t *r = &_SEGGER_RTT.up[0];
  uint32_t wr = r->wr, rd = r->rd;
  return (rd > wr) ? (rd - wr - 1u) : (r->size - (wr - rd) - 1u);
}

bool rtt_write(const char *data, uint32_t len)
{
  rtt_ring_t *r = &_SEGGER_RTT.up[0];
  if (len > rtt_write_free())
    return false;

  uint32_t wr    = r->wr;
  uint32_t first = r->size - wr;
  if (first > len) first = len;
  memcpy(&r->buf[wr], data, first);
  memcpy(&r->buf[0], data + first, len - first);

  wr += len;
  if (wr >= r->size) wr -= r->size;
  __DMB();
  r->wr = wr;
  return true;
}

uint32_t rtt_read(char *buf, uint32_t max)
{
  rtt_ring_t *r = &_SEGGER_RTT.down[0];
  uint32_t wr = r->wr, rd = r->rd, n = 0;
  __DMB();

  while (rd != wr && n < max)
  {
    buf[n++] = r->buf[rd];
    if (++rd >= r->size) rd = 0;
  }
  r->rd = rd;
  return n;
}
