#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

/*
 * ZTS-3000-TR-ECWS-N01 Soil Sensor (Modbus RTU Slave)
 * 토양 온도 / 수분 / 전기전도도 (EC) 3-in-1
 *
 * Protocol:
 *   Address: 0x01, Baud: 9600 bps, 8N1
 *   FC 0x03: Read Holding Registers
 *   Register 0x0000: Moisture (uint16, /10 = %)
 *   Register 0x0001: Temperature (int16, /10 = °C)
 *   Register 0x0002: EC conductivity (uint16, us/cm)
 *
 * Example:
 *   TX: 01 03 00 00 00 03 05 CB
 *   RX: 01 03 06 01 64 FF DD 04 D2 73 DF
 *     → Moisture:    0x0164 = 356 → 35.6%
 *     → Temperature: 0xFFDD = -35 → -3.5°C
 *     → EC:          0x04D2 = 1234 us/cm
 */

#define SENSOR_ADDR        0x01
#define SENSOR_REG_START   0x0000
#define SENSOR_REG_COUNT   3      /* moisture + temperature + EC */
#define SENSOR_READ_INTERVAL_MS  5000  /* 5초 간격 */

struct sensor_data {
	uint16_t mois_raw;    /* 토양 수분 (unsigned, /10 = %) */
	int16_t  temp_raw;    /* 온도 (signed, /10 = °C) */
	uint16_t ec_raw;      /* 전기전도도 (us/cm) */
	uint16_t status;      /* 0=offline, 1=OK, 2=CRC error */
	uint16_t read_count;  /* 성공 읽기 횟수 */
	uint16_t err_count;   /* 오류 횟수 */
};

int sensor_init(void);
int sensor_read(void);
const struct sensor_data *sensor_get_data(void);

#endif
