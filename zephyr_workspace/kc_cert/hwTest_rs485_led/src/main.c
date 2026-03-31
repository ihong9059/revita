#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* LED_EN: P0.15 */
#define PIN_LED_EN     15
/* RS485 DE: P1.04, RE#: P1.03 */
#define PIN_RS485_DE   4
#define PIN_RS485_RE   3

static const struct device *gpio0;
static const struct device *gpio1;
static const struct device *rs485_dev;  /* uart0 */
static uint8_t rx_buf[128];
static volatile int rx_len;

static void rs485_tx_mode(void)
{
	gpio_pin_set_raw(gpio1, PIN_RS485_DE, 1);
	gpio_pin_set_raw(gpio1, PIN_RS485_RE, 1);
}

static void rs485_rx_mode(void)
{
	gpio_pin_set_raw(gpio1, PIN_RS485_DE, 0);
	gpio_pin_set_raw(gpio1, PIN_RS485_RE, 0);
}

static void rs485_send(const char *data, int len)
{
	rs485_tx_mode();
	k_busy_wait(100);

	for (int i = 0; i < len; i++) {
		uart_poll_out(rs485_dev, data[i]);
	}

	k_busy_wait(len * 1100 + 500);
	rs485_rx_mode();
}

static void rs485_rx_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) == 1) {
			if (rx_len < sizeof(rx_buf) - 1) {
				rx_buf[rx_len++] = c;
			}
		}
	}
}

int main(void)
{
	uint32_t count = 0;
	char tx_buf[64];
	bool led_on = false;

	printk("\n========================================\n");
	printk("  HW Test - RS485 + LED\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  RS485: TX=P0.20, RX=P0.19, 9600bps\n");
	printk("         DE=P1.04, RE#=P1.03\n");
	printk("  LED_EN: P0.15 (1초 점멸)\n");
	printk("  TX: 1초마다 카운트 송신\n");
	printk("  RX: 수신 데이터 → 디버그 출력\n");
	printk("========================================\n\n");

	/* GPIO 초기화 */
	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("ERROR: gpio0 not ready\n");
		return -1;
	}

	gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1)) {
		printk("ERROR: gpio1 not ready\n");
		return -1;
	}

	/* LED_EN (P0.15) */
	gpio_pin_configure(gpio0, PIN_LED_EN, GPIO_OUTPUT_LOW);
	printk("LED_EN (P0.15) configured\n");

	/* RS485 DE/RE# */
	gpio_pin_configure(gpio1, PIN_RS485_DE, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_RS485_RE, GPIO_OUTPUT_LOW);
	printk("RS485 DE/RE# configured\n");

	/* RS485 UART (uart0) */
	rs485_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(rs485_dev)) {
		printk("ERROR: uart0 not ready\n");
		return -1;
	}
	printk("RS485 UART ready (9600 bps)\n");

	uart_irq_callback_set(rs485_dev, rs485_rx_cb);
	uart_irq_rx_enable(rs485_dev);
	rs485_rx_mode();

	printk("\nReady.\n\n");

	while (1) {
		/* RS485 수신 데이터 확인 */
		if (rx_len > 0) {
			uart_irq_rx_disable(rs485_dev);
			rx_buf[rx_len] = '\0';
			printk("RS485 RX[%d]: %s\n", rx_len, rx_buf);
			rx_len = 0;
			uart_irq_rx_enable(rs485_dev);
		}

		/* RS485 카운트 송신 */
		snprintf(tx_buf, sizeof(tx_buf), "revita:%u\r\n", count);
		rs485_send(tx_buf, strlen(tx_buf));
		printk("RS485 TX: revita:%u\n", count);
		count++;

		/* LED 토글 */
		led_on = !led_on;
		gpio_pin_set_raw(gpio0, PIN_LED_EN, led_on ? 1 : 0);

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
