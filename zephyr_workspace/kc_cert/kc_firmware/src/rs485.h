#ifndef RS485_H
#define RS485_H

#include <stdint.h>

int rs485_init(void);
void rs485_send(const uint8_t *data, uint16_t len);
int rs485_receive(uint8_t *buf, uint16_t max_len, uint16_t timeout_ms);

#endif
