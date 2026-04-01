#ifndef SENSOR2_H
#define SENSOR2_H

#include <stdint.h>

/*
 * Sensor2: RS485 온습도 센서 (공기)
 *
 * Address: 0x02, Baud: 9600 bps, 8N1
 * FC 0x03: Read Holding Registers
 * Register 0x0000: Humidity (uint16, /10 = %RH)
 * Register 0x0001: Temperature (int16, /10 = °C)
 */

#define SENSOR2_ADDR       0x02
#define SENSOR2_REG_START  0x0000
#define SENSOR2_REG_COUNT  2      /* humidity + temperature */

struct sensor2_data {
	uint16_t humi_raw;    /* 습도 (unsigned, /10 = %RH) */
	int16_t  temp_raw;    /* 온도 (signed, /10 = °C) */
	uint16_t status;      /* 0=offline, 1=OK, 2=error */
	uint16_t read_count;
	uint16_t err_count;
};

int sensor2_init(void);
int sensor2_read(void);
const struct sensor2_data *sensor2_get_data(void);

#endif
