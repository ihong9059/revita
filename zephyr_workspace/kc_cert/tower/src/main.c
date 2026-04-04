#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* ── MCP23017 ── */
#define REG_IODIRA       0x00
#define REG_OLATA        0x14

#define GPA3_MUX_SEL     (1 << 3)
#define GPA4_MUX_EN      (1 << 4)

/* ── RS485 RX buffer ── */
#define RX_BUF_SIZE      256
static uint8_t rx_buf[RX_BUF_SIZE];
static volatile int rx_len = 0;

static const struct device *gpio0_dev;
static const struct device *i2c_dev;
static const struct device *uart_dev;
static uint8_t mcp_addr = 0;

/* ── MCP23017 helpers ── */

static int mcp_write(uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = { reg, val };
	return i2c_write(i2c_dev, buf, 2, mcp_addr);
}

static int mcp_read(uint8_t reg, uint8_t *val)
{
	return i2c_write_read(i2c_dev, mcp_addr, &reg, 1, val, 1);
}

static int mux_enable_rs485(void)
{
	uint8_t iodira, gpa;

	if (mcp_read(REG_IODIRA, &iodira)) return -1;
	iodira &= ~(GPA3_MUX_SEL | GPA4_MUX_EN);
	if (mcp_write(REG_IODIRA, iodira)) return -1;

	if (mcp_read(REG_OLATA, &gpa)) return -1;
	gpa &= ~GPA3_MUX_SEL;
	gpa &= ~GPA4_MUX_EN;
	return mcp_write(REG_OLATA, gpa);
}

/* ── RS485 direction control ── */

static void rs485_tx_mode(void)
{
	gpio_pin_set(gpio0_dev, 21, 1);   /* RE#=HIGH: RX off */
	gpio_pin_set(gpio0_dev, 17, 1);   /* DE=HIGH: TX on */
	k_usleep(100);                     /* 방향 전환 안정화 */
}

static void rs485_rx_mode(void)
{
	k_usleep(500);                     /* 마지막 바이트 송신 완료 대기 */
	gpio_pin_set(gpio0_dev, 17, 0);   /* DE=LOW: TX off */
	gpio_pin_set(gpio0_dev, 21, 0);   /* RE#=LOW: RX on */
}

/* ── RS485 UART send ── */

static void rs485_send(const char *str)
{
	while (*str) {
		uart_poll_out(uart_dev, *str++);
	}
}

/* ── RS485 UART receive (non-blocking, poll) ── */

static void rs485_poll_rx(void)
{
	uint8_t c;
	while (uart_poll_in(uart_dev, &c) == 0) {
		if (rx_len < RX_BUF_SIZE - 1) {
			rx_buf[rx_len++] = c;
		}
	}
}

/* ── Main ── */

int main(void)
{
	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) return -1;

	/* 초기: RX 모드 */
	gpio_pin_configure(gpio0_dev, 17, GPIO_OUTPUT_LOW);   /* DE=LOW */
	gpio_pin_configure(gpio0_dev, 21, GPIO_OUTPUT_LOW);   /* RE#=LOW */

	/* UART device */
	uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart_dev)) return -1;

	/* 부팅 메시지 (TX 모드로 전환해서 출력) */
	rs485_tx_mode();
	rs485_send("\r\n=== Tower v1.7 ===\r\n");

	/* I2C → MCP23017 → MUX */
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(i2c_dev)) {
		rs485_send("I2C: NOT ready\r\n");
		goto start_loop;
	}

	k_msleep(100);

	/* MCP23017 탐색 */
	for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
		uint8_t reg = REG_IODIRA, val;
		if (i2c_write_read(i2c_dev, addr, &reg, 1, &val, 1) == 0) {
			mcp_addr = addr;
			break;
		}
	}

	if (mcp_addr) {
		mux_enable_rs485();
		rs485_send("MUX: RS485 ON\r\n");
	} else {
		rs485_send("MCP: not found\r\n");
	}

start_loop:
	rs485_send("--- start ---\r\n");
	rs485_rx_mode();  /* RX 모드로 전환 */

	uint32_t count = 0;
	char tx_msg[128];

	while (1) {
		/* 1초간 RX 수신 대기 (polling) */
		for (int i = 0; i < 100; i++) {
			rs485_poll_rx();
			k_msleep(10);
		}

		/* TX 모드 전환 → 송신 */
		rs485_tx_mode();

		if (rx_len > 0) {
			rx_buf[rx_len] = '\0';
			snprintf(tx_msg, sizeof(tx_msg),
				"[%u] Tower (RX:%s)\r\n", count++, rx_buf);
			rx_len = 0;
		} else {
			snprintf(tx_msg, sizeof(tx_msg),
				"[%u] Tower\r\n", count++);
		}
		rs485_send(tx_msg);

		/* RX 모드 복귀 */
		rs485_rx_mode();
	}
	return 0;
}
