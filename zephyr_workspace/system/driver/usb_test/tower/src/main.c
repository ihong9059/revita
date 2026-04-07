/*
 * USB CDC ACM basic test - Tower
 *
 * 동작:
 *   1. USB CDC ACM enumerate (호스트에서 /dev/ttyACMx 로 잡힘)
 *   2. 매 2초마다 카운터 메시지 송신 ("[N] Hello from REVITA Tower USB CDC!")
 *   3. 호스트에서 보낸 문자는 그대로 echo 응답 (RX 인터럽트)
 *
 * 출처: zephyr_workspace/usb_cdc (2026-04-06) 패턴 그대로 이식
 * 보드: Tower 의 MCU_USB_D_P / MCU_USB_D_N → USB 커넥터
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

static const struct device *cdc_dev;

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

	cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(cdc_dev)) {
		printk("CDC device not ready\n");
		return -1;
	}

	uart_irq_callback_set(cdc_dev, cdc_irq_handler);
	uart_irq_rx_enable(cdc_dev);

	while (true) {
		printk("[%d] Hello from REVITA Tower USB CDC!\n", count++);
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
