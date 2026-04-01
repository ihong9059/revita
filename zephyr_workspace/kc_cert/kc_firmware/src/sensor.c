/*
 * ZTS-3000 Soil Sensor - Modbus RTU Master
 * 9600bps로 센서(addr 0x01)에서 수분/온도/EC 읽기
 * Host 통신도 9600bps이므로 보레이트 전환 불필요
 */

#include "sensor.h"
#include "rs485.h"
#include "modbus.h"
#include <zephyr/kernel.h>
#include <string.h>

static struct sensor_data sdata;

int sensor_init(void)
{
	memset(&sdata, 0, sizeof(sdata));
	sdata.status = 0;
	return 0;
}

int sensor_read(void)
{
	uint8_t req[8];
	uint8_t resp[32];
	int len;

	/* Build FC 0x03: read 3 registers from 0x0000 */
	req[0] = SENSOR_ADDR;
	req[1] = FC_READ_HOLDING;
	req[2] = (SENSOR_REG_START >> 8) & 0xFF;
	req[3] = SENSOR_REG_START & 0xFF;
	req[4] = (SENSOR_REG_COUNT >> 8) & 0xFF;
	req[5] = SENSOR_REG_COUNT & 0xFF;
	modbus_append_crc(req, 6);

	/* Send request */
	rs485_send(req, 8);
	k_sleep(K_MSEC(2));   /* TX→RX 전환 안정화 */
	rs485_flush_rx();     /* 전환 노이즈 제거 */

	/* Wait for response
	 * Expected: [addr][func][byte_cnt=6][mois_h][mois_l][temp_h][temp_l][ec_h][ec_l][crc_l][crc_h]
	 * = 11 bytes
	 */
	len = rs485_receive(resp, sizeof(resp), 500);

	if (len < 11) {
		sdata.status = 0;
		sdata.err_count++;
		printk("[SENSOR] No response (len=%d)", len);
		if (len > 0) {
			printk(" RX:");
			for (int i = 0; i < len; i++) printk(" %02X", resp[i]);
		}
		printk("\n");
		return -1;
	}

	/* Verify address and function code */
	if (resp[0] != SENSOR_ADDR || resp[1] != FC_READ_HOLDING) {
		if (resp[1] & 0x80) {
			printk("[SENSOR] Exception: 0x%02X\n", resp[2]);
		} else {
			printk("[SENSOR] Bad resp: addr=0x%02X func=0x%02X\n",
			       resp[0], resp[1]);
		}
		sdata.status = 2;
		sdata.err_count++;
		return -2;
	}

	/* Verify CRC */
	if (!modbus_check_crc(resp, len)) {
		printk("[SENSOR] CRC error\n");
		sdata.status = 2;
		sdata.err_count++;
		return -3;
	}

	/* Verify byte count (3 regs × 2 = 6 bytes) */
	if (resp[2] != SENSOR_REG_COUNT * 2) {
		printk("[SENSOR] Bad byte count: %d\n", resp[2]);
		sdata.status = 2;
		sdata.err_count++;
		return -4;
	}

	/* Parse:
	 * resp[3-4] = Moisture (uint16, /10 = %)
	 * resp[5-6] = Temperature (int16, /10 = °C)
	 * resp[7-8] = EC (uint16, us/cm)
	 */
	sdata.mois_raw = (resp[3] << 8) | resp[4];
	sdata.temp_raw = (int16_t)((resp[5] << 8) | resp[6]);
	sdata.ec_raw = (resp[7] << 8) | resp[8];
	sdata.status = 1;
	sdata.read_count++;

	int t_int = sdata.temp_raw / 10;
	int t_dec = (sdata.temp_raw < 0 ? -sdata.temp_raw : sdata.temp_raw) % 10;
	printk("[SENSOR] Mois=%d.%d%%  Temp=%d.%dC  EC=%dus/cm\n",
	       sdata.mois_raw / 10, sdata.mois_raw % 10,
	       t_int, t_dec, sdata.ec_raw);

	return 0;
}

const struct sensor_data *sensor_get_data(void)
{
	return &sdata;
}
