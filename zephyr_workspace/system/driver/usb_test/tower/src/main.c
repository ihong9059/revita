/*
 * USB CDC ACM test - Tower
 *
 * 동작:
 *   1. USB CDC ACM enumerate (호스트에서 /dev/ttyACMx)
 *   2. 매 2초마다 카운터 메시지를 USB CDC + uart0(ttyUSB1) 양쪽에 출력
 *   3. USB CDC에서 수신한 문자는 echo 응답
 *
 * Console: uart0 (P0.20 TX) → ttyUSB1 @ 115200
 * USB CDC: ttyACMx
 *
 * 참고: tower.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

static const struct device *cdc_dev;

static void cdc_send_str(const char *s)
{
	while (*s) {
		uart_poll_out(cdc_dev, *s++);
	}
}

static void cdc_irq_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		uint8_t buf[64];
		int len;

		if (uart_irq_rx_ready(dev)) {
			len = uart_fifo_read(dev, buf, sizeof(buf));
			for (int i = 0; i < len; i++) {
				uart_poll_out(dev, buf[i]);
			}
		}
	}
}

int main(void)
{
	int count = 0;
	char msg[80];

	printk("\n========================================\n");
	printk("  USB CDC ACM test - Tower\n");
	printk("  Console: uart0 (ttyUSB1 @ 115200)\n");
	printk("  USB CDC: ttyACMx\n");
	printk("========================================\n");

	cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(cdc_dev)) {
		printk("[USB] CDC device not ready\n");
		return -1;
	}
	printk("[USB] CDC device ready\n");

	uart_irq_callback_set(cdc_dev, cdc_irq_handler);
	uart_irq_rx_enable(cdc_dev);

	while (true) {
		/* uart0 (ttyUSB1) */
		printk("[UART] #%d Hello from REVITA Tower\n", count);

		/* USB CDC (ttyACMx) */
		snprintf(msg, sizeof(msg),
			 "[USB] #%d Hello from REVITA Tower\n", count);
		cdc_send_str(msg);

		count++;

		k_sleep(K_SECONDS(2));
	}

	return 0;
}
