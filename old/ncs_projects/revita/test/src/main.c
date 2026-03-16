#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/console/console.h>
#include <string.h>
#include <stdio.h>

#define MAX_LINE_LEN 128
#define LED_THREAD_STACK_SIZE 1024
#define LED_THREAD_PRIORITY 5

/* nRF52840 DK has 4 LEDs */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

/* UART1 device */
static const struct device *uart1_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

static char rx_buf[MAX_LINE_LEN];
static int rx_idx = 0;

/* LED blink thread stack */
K_THREAD_STACK_DEFINE(led_thread_stack, LED_THREAD_STACK_SIZE);
static struct k_thread led_thread_data;

/* Send string via UART1 */
static void uart1_send(const char *str)
{
	while (*str) {
		uart_poll_out(uart1_dev, *str++);
	}
}

/* LED blink and counter thread function */
static void led_blink_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	uint32_t counter = 0;
	char buf[64];

	while (1) {
		/* Turn all LEDs on */
		for (int i = 0; i < ARRAY_SIZE(leds); i++) {
			gpio_pin_set_dt(&leds[i], 1);
		}
		k_sleep(K_MSEC(50));  /* 10x faster: 50ms instead of 500ms */

		/* Turn all LEDs off */
		for (int i = 0; i < ARRAY_SIZE(leds); i++) {
			gpio_pin_set_dt(&leds[i], 0);
		}
		k_sleep(K_MSEC(50));  /* 10x faster: 50ms instead of 500ms */

		counter++;

		/* Output to UART0 (console) */
		printk("Counter: %u\n", counter);

		/* Output to UART1 */
		snprintf(buf, sizeof(buf), "UART2 Counter: %u\r\n", counter);
		uart1_send(buf);
	}
}

int main(void)
{
	int ret;

	/* Initialize all LEDs */
	for (int i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!gpio_is_ready_dt(&leds[i])) {
			return -1;
		}
		ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return -1;
		}
	}

	/* Check UART1 device */
	if (!device_is_ready(uart1_dev)) {
		printk("UART1 device not ready!\n");
		return -1;
	}

	/* Initialize console for input */
	console_init();

	printk("UART Test Started\n");
	printk("UART0: Console + Echo\n");
	printk("UART1 (P1.01 TX, P1.02 RX): Counter output\n");
	printk("Type something and press Enter:\n");

	/* Send init message to UART1 */
	uart1_send("UART2 Initialized\r\n");

	/* Start LED blink thread */
	k_thread_create(&led_thread_data, led_thread_stack,
			K_THREAD_STACK_SIZEOF(led_thread_stack),
			led_blink_thread, NULL, NULL, NULL,
			LED_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Main loop: handle UART input */
	while (1) {
		int c = console_getchar();

		if (c == '\r' || c == '\n') {
			rx_buf[rx_idx] = '\0';
			if (rx_idx > 0) {
				printk("resend: %s\n", rx_buf);
			}
			rx_idx = 0;
		} else if (rx_idx < MAX_LINE_LEN - 1) {
			rx_buf[rx_idx++] = (char)c;
		}
	}

	return 0;
}
