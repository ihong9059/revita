#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

void reg_init(void);
int32_t reg_read(uint16_t addr);
int reg_write(uint16_t addr, uint16_t value);
void reg_update_sensors(void);

#endif
