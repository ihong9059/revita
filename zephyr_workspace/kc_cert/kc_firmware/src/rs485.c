#include "rs485.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>

/* RS485 DE: P1.04, RE#: P1.03 */
#define PIN_RS485_DE  4
#define PIN_RS485_RE  3

/* Frame timeout: 3.5 char @ 9600bps = ~4ms, use 5ms */
#define FRAME_TIMEOUT_MS  5

static const struct device *uart_dev;
static const struct device *gpio1;

static uint8_t rx_buf[256];
static volatile uint16_t rx_pos;
static struct k_sem rx_sem;
static volatile int64_t last_rx_time;

static void rx_mode(void)
{
	gpio_pin_set_raw(gpio1, PIN_RS485_DE, 0);
	gpio_pin_set_raw(gpio1, PIN_RS485_RE, 0);
}

static void tx_mode(void)
{
	gpio_pin_set_raw(gpio1, PIN_RS485_DE, 1);
	gpio_pin_set_raw(gpio1, PIN_RS485_RE, 1);
}

static void uart_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) == 1) {
			if (rx_pos < sizeof(rx_buf)) {
				rx_buf[rx_pos++] = c;
				last_rx_time = k_uptime_get();
			}
		}
	}
}

int rs485_init(void)
{
	/* GPIO1 for DE/RE# */
	gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1)) {
		printk("ERROR: gpio1 not ready\n");
		return -1;
	}
	gpio_pin_configure(gpio1, PIN_RS485_DE, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_RS485_RE, GPIO_OUTPUT_LOW);

	/* UART0 for RS485 */
	uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart_dev)) {
		printk("ERROR: uart0 not ready\n");
		return -1;
	}

	k_sem_init(&rx_sem, 0, 1);
	rx_pos = 0;
	last_rx_time = 0;

	uart_irq_callback_set(uart_dev, uart_cb);
	uart_irq_rx_enable(uart_dev);
	rx_mode();

	return 0;
}

void rs485_send(const uint8_t *data, uint16_t len)
{
	tx_mode();
	k_busy_wait(100);

	for (uint16_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, data[i]);
	}

	/* Wait TX complete: ~1.04ms per byte @ 9600 */
	k_busy_wait((uint32_t)len * 1100 + 500);
	rx_mode();
}

int rs485_receive(uint8_t *buf, uint16_t max_len, uint16_t timeout_ms)
{
	int64_t start = k_uptime_get();

	/* Wait for first byte or timeout */
	while (rx_pos == 0) {
		if (k_uptime_get() - start > timeout_ms) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	}

	/* Wait for frame complete (silence > FRAME_TIMEOUT_MS) */
	while (1) {
		k_sleep(K_MSEC(1));
		int64_t now = k_uptime_get();

		if (now - last_rx_time >= FRAME_TIMEOUT_MS && rx_pos > 0) {
			break;
		}
		if (now - start > timeout_ms) {
			break;
		}
	}

	/* Copy frame */
	uart_irq_rx_disable(uart_dev);
	uint16_t len = rx_pos;
	if (len > max_len) {
		len = max_len;
	}
	memcpy(buf, (const void *)rx_buf, len);
	rx_pos = 0;
	uart_irq_rx_enable(uart_dev);

	return len;
}
