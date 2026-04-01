#include <zephyr/kernel.h>
#include "rs485.h"
#include "modbus.h"
#include "registers.h"
#include "drivers.h"
#include "lora.h"
#include "sensor.h"
#include "sensor2.h"

int main(void)
{
	uint8_t frame[MODBUS_MAX_FRAME];
	int len;

	printk("\n========================================\n");
	printk("  KC Certification Firmware v1.0\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  RS485: 9600 bps (Modbus RTU Slave)\n");
	printk("  LoRa:  922.1 MHz (End Device RX)\n");
	printk("  Slave Address: 0x10\n");
	printk("  Sensor Address: 0x01 (Temp/Humi)\n");
	printk("========================================\n\n");

	/* Init drivers */
	if (drv_init() < 0) {
		printk("Driver init failed\n");
		return -1;
	}

	/* Init registers */
	reg_init();

	/* Init RS485 */
	if (rs485_init() < 0) {
		printk("RS485 init failed\n");
		return -1;
	}

	/* 12V 센서 전원 ON */
	drv_12v_set(true);
	printk("12V supply ON (sensor power)\n");
	k_sleep(K_SECONDS(3));  /* 센서 안정화 3초 */

	/* Init Sensors */
	sensor_init();
	sensor2_init();

	/* Init LoRa */
	if (lora_module_init() < 0) {
		printk("LoRa init failed (continuing without LoRa)\n");
	} else {
		/* Auto-start RX */
		lora_rx_start();
	}

	/* Initial GPIO read for debug */
	printk("BTN(P0.05) initial: %d\n", drv_btn_read());
	printk("VIB(P0.21) initial: %d\n", drv_vib_read());
	printk("\nReady. Waiting for Modbus commands...\n\n");

	uint32_t loop_count = 0;
	int64_t next_sensor_read = k_uptime_get() + 2000;
	int64_t last_host_cmd = 0;  /* 마지막 Host 명령 시각 */

	while (1) {
		/* Receive Modbus frame (100ms timeout) */
		len = rs485_receive(frame, sizeof(frame), 100);
		if (len > 0) {
			modbus_process_frame(frame, len);
			last_host_cmd = k_uptime_get();
			/* Host 명령 후 센서 읽기 지연 (충돌 방지) */
			next_sensor_read = last_host_cmd + 2000;
		}

		/* Update sensor readings & timers */
		reg_update_sensors();

		/* Update LoRa status */
		reg_update_lora();

		/* 주기적 외부 센서 읽기
		 * 조건: Host 명령 후 2초 이상 경과 시에만 */
		int64_t now = k_uptime_get();
		if (now >= next_sensor_read &&
		    (now - last_host_cmd) >= 2000) {
			sensor_read();
			reg_update_sensor();
			k_sleep(K_MSEC(100));  /* 버스 안정화 */
			sensor2_read();
			reg_update_sensor2();
			next_sensor_read = now + SENSOR_READ_INTERVAL_MS;
		}

		/* LED blink tick (~100ms) */
		drv_led_tick();

		/* 10초마다 상태 출력 (디버그) */
		loop_count++;
		if (loop_count % 100 == 0) {
			const struct sensor_data *sd = sensor_get_data();
			printk("[DBG] BTN=%d VIB=%d | Soil: %d.%d%% %d.%dC EC=%d (st=%d)\n",
			       drv_btn_read(), drv_vib_read(),
			       sd->mois_raw / 10, sd->mois_raw % 10,
			       sd->temp_raw / 10,
			       (sd->temp_raw < 0 ? -sd->temp_raw : sd->temp_raw) % 10,
			       sd->ec_raw, sd->status);
		}
	}

	return 0;
}
