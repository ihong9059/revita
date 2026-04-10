/*
 * Button & Vibration sensor test - Tower
 *
 * BTN:        P1.02 (SW2) - pull-up, active low
 * VIB_SENSE:  P1.04 (LED2) - pull-down, active high
 *
 * 100ms 폴링으로 상태 변화 감지 시 출력
 * 1초마다 현재 상태 출력
 *
 * 참고: tower.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define PIN_BTN        2   /* P1.02 */
#define PIN_VIB_SENSE  4   /* P1.04 */

int main(void)
{
	const struct device *gpio1;
	int btn_prev = -1, vib_prev = -1;
	uint32_t tick = 0;
	uint32_t btn_count = 0, vib_count = 0;

	printk("\n========================================\n");
	printk("  Input test - Tower\n");
	printk("  BTN       = P1.02 (pull-up, active low)\n");
	printk("  VIB_SENSE = P1.04 (pull-down, active high)\n");
	printk("  poll: 100ms, status: 1s\n");
	printk("========================================\n");

	gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1)) {
		printk("[INPUT] gpio1 NOT ready\n");
		return -1;
	}

	gpio_pin_configure(gpio1, PIN_BTN, GPIO_INPUT | GPIO_PULL_UP);
	gpio_pin_configure(gpio1, PIN_VIB_SENSE, GPIO_INPUT | GPIO_PULL_DOWN);
	printk("[INPUT] pins configured\n");

	while (1) {
		int btn = gpio_pin_get_raw(gpio1, PIN_BTN);
		int vib = gpio_pin_get_raw(gpio1, PIN_VIB_SENSE);

		if (btn != btn_prev) {
			btn_count++;
			printk("[INPUT] BTN %s (count=%u)\n",
			       btn ? "RELEASED" : "PRESSED", btn_count);
			btn_prev = btn;
		}

		if (vib != vib_prev) {
			vib_count++;
			printk("[INPUT] VIB %s (count=%u)\n",
			       vib ? "ACTIVE" : "IDLE", vib_count);
			vib_prev = vib;
		}

		tick++;
		if (tick % 10 == 0) {
			printk("[INPUT] status: BTN=%d VIB=%d (t=%u)\n",
			       btn, vib, tick / 10);
		}

		k_msleep(100);
	}

	return 0;
}
