#include <zephyr/kernel.h>
#include "rs485.h"
#include "modbus.h"
#include "registers.h"
#include "drivers.h"

int main(void)
{
	uint8_t frame[MODBUS_MAX_FRAME];
	int len;

	printk("\n========================================\n");
	printk("  KC Certification Firmware v1.0\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  RS485: 9600 bps (Modbus RTU Slave)\n");
	printk("  Slave Address: 0x01\n");
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

	/* Initial GPIO read for debug */
	printk("BTN(P0.05) initial: %d\n", drv_btn_read());
	printk("VIB(P0.21) initial: %d\n", drv_vib_read());
	printk("\nReady. Waiting for Modbus commands...\n\n");

	uint32_t loop_count = 0;

	while (1) {
		/* Receive Modbus frame (100ms timeout) */
		len = rs485_receive(frame, sizeof(frame), 100);
		if (len > 0) {
			modbus_process_frame(frame, len);
		}

		/* Update sensor readings & timers */
		reg_update_sensors();

		/* LED blink tick (~100ms) */
		drv_led_tick();

		/* 5초마다 입력 상태 출력 (디버그) */
		loop_count++;
		if (loop_count % 50 == 0) {
			printk("[DBG] BTN=%d VIB=%d vib_cnt=%d\n",
			       drv_btn_read(), drv_vib_read(), 0);
		}
	}

	return 0;
}
