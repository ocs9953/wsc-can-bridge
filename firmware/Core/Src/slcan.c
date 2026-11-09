#include "slcan.h"
#include <stdio.h>
#include <string.h>
#include "stm32u5xx_hal.h"
#include "can_if.h"
#include "rtt.h"

#define FW_VERSION      "1.0"
#define LINE_MAX        64u
#define STATUS_PERIOD   1000u
#define HOST_TIMEOUT    3000u

static const uint32_t sn_bitrate[] = {
  10000u, 20000u, 50000u, 100000u, 125000u, 250000u, 500000u, 800000u, 1000000u
};

static char     line[LINE_MAX];
static uint32_t line_len;
static bool     line_overflow;
static uint32_t rtt_drop;
static uint32_t last_status;
static uint32_t last_host;

static const char hex[] = "0123456789ABCDEF";


static void out(const char *s, uint32_t n)
{
  if (!rtt_write(s, n))
    rtt_drop++;
}

static void out_str(const char *s) { out(s, (uint32_t)strlen(s)); }

static uint32_t format_frame(const can_frame_t *f, char *b)
{
  uint32_t n = 0;
  if (f->ext)
  {
    b[n++] = f->rtr ? 'R' : 'T';
    for (int s = 28; s >= 0; s -= 4)
      b[n++] = hex[(f->id >> s) & 0xFu];
  }
  else
  {
    b[n++] = f->rtr ? 'r' : 't';
    b[n++] = hex[(f->id >> 8) & 0x7u];
    b[n++] = hex[(f->id >> 4) & 0xFu];
    b[n++] = hex[f->id & 0xFu];
  }
  b[n++] = (char)('0' + f->len);
  if (!f->rtr)
  {
    for (uint32_t i = 0; i < f->len; i++)
    {
      b[n++] = hex[f->data[i] >> 4];
      b[n++] = hex[f->data[i] & 0xFu];
    }
  }
  b[n++] = '\r';
  return n;
}

static void send_status(void)
{
  can_stats_t s;
  uint8_t tec, rec;
  bool ep, bo;
  can_get_stats(&s);
  can_get_errors(&tec, &rec, &ep, &bo);

  static const char *const mode_name[] = { "closed", "open", "listen" };
  char b[200];
  int n = snprintf(b, sizeof(b),
    "#S mode=%s br=%lu rx=%lu tx=%lu rxlost=%lu txdrop=%lu busoff=%lu "
    "tec=%u rec=%u ep=%u bo=%u rttdrop=%lu\r",
    mode_name[can_mode()], (unsigned long)can_get_bitrate(),
    (unsigned long)s.rx, (unsigned long)s.tx,
    (unsigned long)(s.rx_lost_hw + s.rx_lost_sw), (unsigned long)s.tx_drop,
    (unsigned long)s.busoff, tec, rec, ep ? 1u : 0u, bo ? 1u : 0u,
    (unsigned long)rtt_drop);
  if (n > 0)
    out(b, (uint32_t)n);
}


static int hexval(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static bool parse_hex(const char *s, uint32_t digits, uint32_t *v)
{
  uint32_t x = 0;
  for (uint32_t i = 0; i < digits; i++)
  {
    int h = hexval(s[i]);
    if (h < 0) return false;
    x = (x << 4) | (uint32_t)h;
  }
  *v = x;
  return true;
}

static bool parse_frame(const char *s, uint32_t n, can_frame_t *f)
{
  memset(f, 0, sizeof(*f));
  f->ext = (s[0] == 'T' || s[0] == 'R');
  f->rtr = (s[0] == 'r' || s[0] == 'R');
  uint32_t id_digits = f->ext ? 8u : 3u;

  if (n < 1u + id_digits + 1u)
    return false;
  if (!parse_hex(&s[1], id_digits, &f->id))
    return false;
  if (f->id > (f->ext ? 0x1FFFFFFFu : 0x7FFu))
    return false;

  int len = hexval(s[1 + id_digits]);
  if (len < 0 || len > 8)
    return false;
  f->len = (uint8_t)len;

  uint32_t p = 2u + id_digits;
  if (f->rtr)
    return n == p;
  if (n != p + 2u * f->len)
    return false;
  for (uint32_t i = 0; i < f->len; i++)
  {
    uint32_t v;
    if (!parse_hex(&s[p + 2u * i], 2u, &v))
      return false;
    f->data[i] = (uint8_t)v;
  }
  return true;
}

static void handle_line(const char *s, uint32_t n)
{
  static const char ok[] = "\r", bell[] = "\a";
  can_frame_t f;

  switch (s[0])
  {
  case 'S':
    if (n == 2 && s[1] >= '0' && s[1] <= '8' && can_set_bitrate(sn_bitrate[s[1] - '0']))
      out_str(ok);
    else
      out_str(bell);
    break;

  case 'O':
  case 'L':
    out_str(can_open(s[0] == 'L') ? ok : bell);
    break;

  case 'C':
    can_close();
    out_str(ok);
    break;

  case 't': case 'T': case 'r': case 'R':
    if (parse_frame(s, n, &f) && can_send(&f))
      out_str(f.ext ? "Z\r" : "z\r");
    else
      out_str(bell);
    break;

  case 'F': {
    uint8_t tec, rec;
    bool ep, bo;
    can_get_errors(&tec, &rec, &ep, &bo);
    uint8_t flags = (uint8_t)(((tec >= 96u || rec >= 96u) ? 0x04u : 0u) |
                              (ep ? 0x20u : 0u) | (bo ? 0x80u : 0u));
    char b[5] = { 'F', hex[flags >> 4], hex[flags & 0xFu], '\r', 0 };
    out_str(b);
    break;
  }

  case 'V': out_str("V0100\r");  break;
  case 'N': out_str("NWSC1\r");  break;
  case 'Z': out_str(ok);         break;

  case '#':
    if (n >= 2 && s[1] == 'E')
    {
      char b[LINE_MAX + 1];
      memcpy(b, s, n);
      b[n] = '\r';
      out(b, n + 1u);
    }
    break;

  default:
    out_str(bell);
    break;
  }
}

static void poll_input(void)
{
  char buf[64];
  uint32_t n;
  while ((n = rtt_read(buf, sizeof(buf))) > 0u)
  {
    last_host = HAL_GetTick();
    for (uint32_t i = 0; i < n; i++)
    {
      char c = buf[i];
      if (c == '\r' || c == '\n')
      {
        if (!line_overflow && line_len > 0u)
          handle_line(line, line_len);
        line_len = 0;
        line_overflow = false;
      }
      else if (line_len < LINE_MAX)
      {
        line[line_len++] = c;
      }
      else
      {
        line_overflow = true;
      }
    }
  }
}

static void poll_output(void)
{
  can_frame_t f;
  char b[32];
  while (can_peek(&f))
  {
    uint32_t n = format_frame(&f, b);
    if (!rtt_write(b, n))
      break;
    can_pop();
  }
}


void slcan_init(void)
{
  char b[96];
  int n = snprintf(b, sizeof(b), "#BOOT WSC-CAN-Bridge fw=%s fdcan_clk=%lu\r",
                   FW_VERSION, (unsigned long)HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN1));
  if (n > 0)
    out(b, (uint32_t)n);
  last_status = HAL_GetTick();
}

void slcan_poll(void)
{
  poll_input();

  uint32_t now = HAL_GetTick();
  if (can_mode() != CAN_CLOSED && now - last_host > HOST_TIMEOUT)
  {
    can_close();
    out_str("#TIMEOUT\r");
  }

  can_poll();
  poll_output();

  if (now - last_status >= STATUS_PERIOD)
  {
    last_status = now;
    send_status();
  }
}
