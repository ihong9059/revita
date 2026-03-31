#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* BTN: P0.05 - Active Low, Pull-up */
#define PIN_BTN        5
/* VIB_SENSE: P0.21 */
#define PIN_VIB_SENSE  21

#define POLL_INTERVAL_MS  50

int main(void)
{
	const struct device *gpio0;
	int btn_prev, btn_cur;
	int vib_prev, vib_cur;

	printk("\n========================================\n");
	printk("  HW Test - Digital Input (BTN, VIB)\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  BTN:       P0.05 (Active High, Pull-down)\n");
	printk("  VIB_SENSE: P0.21\n");
	printk("  Polling: %d ms\n", POLL_INTERVAL_MS);
	printk("========================================\n\n");

	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("ERROR: gpio0 not ready\n");
		return -1;
	}

	gpio_pin_configure(gpio0, PIN_BTN, GPIO_INPUT | GPIO_PULL_DOWN);
	gpio_pin_configure(gpio0, PIN_VIB_SENSE, GPIO_INPUT | GPIO_PULL_UP);

	btn_prev = gpio_pin_get_raw(gpio0, PIN_BTN);
	vib_prev = gpio_pin_get_raw(gpio0, PIN_VIB_SENSE);

	printk("BTN initial: %s\n", btn_prev ? "PRESSED (HIGH)" : "RELEASED (LOW)");
	printk("VIB initial: %s\n", vib_prev ? "HIGH" : "LOW");
	printk("\nReady. Waiting for changes...\n\n");

	while (1) {
		btn_cur = gpio_pin_get_raw(gpio0, PIN_BTN);
		if (btn_cur != btn_prev) {
			printk("BTN (P0.05): %s\n",
			       btn_cur ? "PRESSED (HIGH)" : "RELEASED (LOW)");
			btn_prev = btn_cur;
		}

		vib_cur = gpio_pin_get_raw(gpio0, PIN_VIB_SENSE);
		if (vib_cur != vib_prev) {
			printk("VIB (P0.21): %s\n",
			       vib_cur ? "HIGH" : "LOW - 진동감지");
			vib_prev = vib_cur;
		}

		k_sleep(K_MSEC(POLL_INTERVAL_MS));
	}

	return 0;
}
