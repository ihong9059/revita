/*
 * RS485 basic test - Node (Link)
 *
 * 동작:
 *   1. 부팅 시 12V_EN(P0.17) HIGH → RS485 트랜시버 전원 ON
 *   2. 매 2초마다 "ND-RS485:<count>\r\n" 송신 (DE/RE# HIGH → TX → DE/RE# LOW)
 *   3. 송신 후 RX 모드로 돌아가 수신 인터럽트로 들어오는 데이터 수집
 *   4. 모든 동작은 디버그 콘솔(ttyUSB0)에 출력
 *
 * 참고: link.h, kc_cert/kc_firmware/src/rs485.c
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* ── Pin numbers (link.h 기준, port-relative) ── */
#define PIN_12V_EN_PORT0    17  /* P0.17 */
#define PIN_RS485_DE_PORT1   4  /* P1.04 */
#define PIN_RS485_RE_PORT1   3  /* P1.03 */

#define TX_INTERVAL_MS      2000
#define RX_BUF_SZ           128

static const struct device *uart_rs485;
static const struct device *gpio0_dev;
static const struct device *gpio1_dev;

/* RX 버퍼 (uart IRQ에서 채움) */
static uint8_t rx_buf[RX_BUF_SZ];
static volatile uint16_t rx_pos;
static volatile int64_t last_rx_time;

static void rs485_tx_mode(void)
{
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_RE_PORT1, 1);  /* RE# = HIGH (RX off) */
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 1);  /* DE  = HIGH (TX on)  */
	k_busy_wait(100);
}

static void rs485_rx_mode(void)
{
	/* 9600 baud: 1 byte ≈ 1.04 ms. uart_poll_out 이 ENDTX 까지만 기다리므로
	 * shift register 에서 마지막 byte 가 빠져나가는 시간을 추가 확보. */
	k_busy_wait(2000);
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 0);   /* DE  = LOW (TX off) */
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_RE_PORT1, 0);   /* RE# = LOW (RX on)  */
}

static void uart_rs485_isr(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}
	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) == 1) {
			if (rx_pos < RX_BUF_SZ) {
				rx_buf[rx_pos++] = c;
				last_rx_time = k_uptime_get();
			}
		}
	}
}

static void rs485_send_str(const char *s)
{
	rs485_tx_mode();
	while (*s) {
		uart_poll_out(uart_rs485, *s++);
	}
	rs485_rx_mode();
}

static void dump_rx_if_any(void)
{
	if (rx_pos == 0) {
		return;
	}
	/* 5 ms 동안 추가 바이트 없으면 프레임 종료로 간주 */
	if (k_uptime_get() - last_rx_time < 5) {
		return;
	}

	uart_irq_rx_disable(uart_rs485);
	uint16_t n = rx_pos;
	printk("[ND-RS485-RX] %u bytes:", n);
	for (uint16_t i = 0; i < n; i++) {
		printk(" %02X", rx_buf[i]);
	}
	printk("  \"");
	for (uint16_t i = 0; i < n; i++) {
		uint8_t c = rx_buf[i];
		printk("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
	}
	printk("\"\n");
	rx_pos = 0;
	uart_irq_rx_enable(uart_rs485);
}

int main(void)
{
	uint32_t count = 0;
	char tx_str[64];

	printk("\n========================================\n");
	printk("  RS485 basic test - Node (Link)\n");
	printk("  uart0 (P0.20/P0.19) @ 9600 bps\n");
	printk("  DE=P1.04 RE#=P1.03 12V_EN=P0.17\n");
	printk("========================================\n");

	/* GPIO0 (P0.17 = 12V_EN) */
	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		printk("[ND] gpio0 NOT ready\n");
		return -1;
	}
	gpio_pin_configure(gpio0_dev, PIN_12V_EN_PORT0, GPIO_OUTPUT_LOW);

	/* GPIO1 (P1.04 = DE, P1.03 = RE#) */
	gpio1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1_dev)) {
		printk("[ND] gpio1 NOT ready\n");
		return -1;
	}
	gpio_pin_configure(gpio1_dev, PIN_RS485_DE_PORT1, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1_dev, PIN_RS485_RE_PORT1, GPIO_OUTPUT_LOW);

	/* 12V 전원 ON */
	gpio_pin_set_raw(gpio0_dev, PIN_12V_EN_PORT0, 1);
	printk("[ND] 12V_EN = HIGH (RS485 transceiver power ON)\n");
	k_msleep(50);

	/* RS485 UART (uart0) */
	uart_rs485 = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart_rs485)) {
		printk("[ND] uart0 NOT ready\n");
		return -1;
	}
	uart_irq_callback_set(uart_rs485, uart_rs485_isr);
	uart_irq_rx_enable(uart_rs485);
	rs485_rx_mode();
	printk("[ND] uart0 ready, entering RX mode\n");

	while (1) {
		/* === TX === */
		int len = snprintf(tx_str, sizeof(tx_str),
				   "ND-RS485:%u\r\n", count);
		printk("\n[ND-RS485-TX] #%u sending \"ND-RS485:%u\" (%d bytes)\n",
		       count, count, len);
		rs485_send_str(tx_str);
		printk("[ND-RS485-TX] sent OK\n");
		count++;

		/* === RX 수집 (TX 간격 동안) === */
		for (int i = 0; i < TX_INTERVAL_MS / 50; i++) {
			k_msleep(50);
			dump_rx_if_any();
		}
	}

	return 0;
}
