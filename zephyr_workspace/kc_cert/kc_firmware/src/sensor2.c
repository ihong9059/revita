/*
 * Sensor2: RS485 온습도 센서 - Modbus RTU Master
 * Address 0x02, 9600bps
 */

#include "sensor2.h"
#include "rs485.h"
#include "modbus.h"
#include <zephyr/kernel.h>
#include <string.h>

static struct sensor2_data sdata;

int sensor2_init(void)
{
	memset(&sdata, 0, sizeof(sdata));
	return 0;
}

int sensor2_read(void)
{
	uint8_t req[8];
	uint8_t resp[32];
	int len;

	/* FC 0x03: read 2 registers from 0x0000 */
	req[0] = SENSOR2_ADDR;
	req[1] = FC_READ_HOLDING;
	req[2] = (SENSOR2_REG_START >> 8) & 0xFF;
	req[3] = SENSOR2_REG_START & 0xFF;
	req[4] = (SENSOR2_REG_COUNT >> 8) & 0xFF;
	req[5] = SENSOR2_REG_COUNT & 0xFF;
	modbus_append_crc(req, 6);

	rs485_send(req, 8);
	k_sleep(K_MSEC(2));
	rs485_flush_rx();

	/* Expected: [addr][func][byte_cnt=4][humi_h][humi_l][temp_h][temp_l][crc_l][crc_h] = 9 bytes */
	len = rs485_receive(resp, sizeof(resp), 500);

	if (len < 9) {
		sdata.status = 0;
		sdata.err_count++;
		printk("[SENSOR2] No response (len=%d)", len);
		if (len > 0) {
			printk(" RX:");
			for (int i = 0; i < len; i++) {
				printk(" %02X", resp[i]);
			}
		}
		printk("\n");
		return -1;
	}

	if (resp[0] != SENSOR2_ADDR || resp[1] != FC_READ_HOLDING) {
		if (resp[1] & 0x80) {
			printk("[SENSOR2] Exception: 0x%02X\n", resp[2]);
		} else {
			printk("[SENSOR2] Bad resp: addr=0x%02X func=0x%02X\n",
			       resp[0], resp[1]);
		}
		sdata.status = 2;
		sdata.err_count++;
		return -2;
	}

	if (!modbus_check_crc(resp, len)) {
		printk("[SENSOR2] CRC error\n");
		sdata.status = 2;
		sdata.err_count++;
		return -3;
	}

	if (resp[2] != SENSOR2_REG_COUNT * 2) {
		printk("[SENSOR2] Bad byte count: %d\n", resp[2]);
		sdata.status = 2;
		sdata.err_count++;
		return -4;
	}

	/* Parse: resp[3-4]=Humidity, resp[5-6]=Temperature */
	sdata.humi_raw = (resp[3] << 8) | resp[4];
	sdata.temp_raw = (int16_t)((resp[5] << 8) | resp[6]);
	sdata.status = 1;
	sdata.read_count++;

	int t_int = sdata.temp_raw / 10;
	int t_dec = (sdata.temp_raw < 0 ? -sdata.temp_raw : sdata.temp_raw) % 10;
	printk("[SENSOR2] Humi=%d.%d%%RH  Temp=%d.%dC\n",
	       sdata.humi_raw / 10, sdata.humi_raw % 10,
	       t_int, t_dec);

	return 0;
}

const struct sensor2_data *sensor2_get_data(void)
{
	return &sdata;
}
