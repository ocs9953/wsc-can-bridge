#ifndef RTT_H
#define RTT_H

#include <stdbool.h>
#include <stdint.h>

void     rtt_init(void);

bool     rtt_write(const char *data, uint32_t len);
uint32_t rtt_write_free(void);

uint32_t rtt_read(char *buf, uint32_t max);

#endif
