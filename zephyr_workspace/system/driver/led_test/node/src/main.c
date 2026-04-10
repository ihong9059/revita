/*
 * LED blink test - Node (Link)
 *
 * LED: P0.15 (UART2_RX와 겸용, GPIO 출력으로 사용)
 * 1초 간격으로 ON/OFF 토글, 디버그 콘솔에 상태 출력
 *
 * 참고: link.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define PIN_LED  15  /* P0.15 */

int main(void)
{
	const struct device *gpio0;
	bool led_on = false;
	uint32_t count = 0;

	printk("\n========================================\n");
	printk("  LED blink test - Node (Link)\n");
	printk("  LED = P0.15 (UART2_RX 겸용)\n");
	printk("  pattern: 1s ON / 1s OFF\n");
	printk("========================================\n");

	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("[LED] gpio0 NOT ready\n");
		return -1;
	}

	gpio_pin_configure(gpio0, PIN_LED, GPIO_OUTPUT_LOW);
	printk("[LED] P0.15 configured as output\n");

	while (1) {
		led_on = !led_on;
		gpio_pin_set_raw(gpio0, PIN_LED, led_on ? 1 : 0);
		count++;
		printk("[LED] #%u %s\n", count, led_on ? "ON" : "OFF");
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
