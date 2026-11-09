#ifndef CAN_IF_H
#define CAN_IF_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t id;
  uint8_t  ext;
  uint8_t  rtr;
  uint8_t  len;
  uint8_t  data[8];
} can_frame_t;

typedef enum { CAN_CLOSED = 0, CAN_OPEN_NORMAL, CAN_OPEN_LISTEN } can_mode_t;

typedef struct {
  uint32_t rx;
  uint32_t tx;
  uint32_t rx_lost_hw;
  uint32_t rx_lost_sw;
  uint32_t tx_drop;
  uint32_t busoff;
} can_stats_t;

void        can_park_pins(void);

bool        can_set_bitrate(uint32_t bps);
uint32_t    can_get_bitrate(void);

bool        can_open(bool listen_only);
void        can_close(void);
can_mode_t  can_mode(void);

bool        can_send(const can_frame_t *f);
bool        can_peek(can_frame_t *f);
void        can_pop(void);

void        can_poll(void);

void        can_get_stats(can_stats_t *s);
void        can_get_errors(uint8_t *tec, uint8_t *rec, bool *passive, bool *busoff);

#endif
