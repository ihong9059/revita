#ifndef MODBUS_H
#define MODBUS_H

#include <stdint.h>
#include <stdbool.h>

#define MODBUS_SLAVE_ADDR  0x01
#define MODBUS_MAX_FRAME   256

/* Function codes */
#define FC_READ_HOLDING    0x03
#define FC_WRITE_SINGLE    0x06
#define FC_WRITE_MULTIPLE  0x10

/* Exception codes */
#define EX_ILLEGAL_FUNC    0x01
#define EX_ILLEGAL_ADDR    0x02
#define EX_ILLEGAL_VALUE   0x03
#define EX_DEVICE_BUSY     0x06

uint16_t modbus_crc16(const uint8_t *data, uint16_t len);
bool modbus_check_crc(const uint8_t *frame, uint16_t len);
void modbus_append_crc(uint8_t *frame, uint16_t len);
void modbus_process_frame(const uint8_t *frame, uint16_t len);

#endif
