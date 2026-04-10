/*
 * Soil sensor Modbus RTU test - Node (Link)
 *
 * 토양센서 (slave 0x01) polling:
 *   요청: 01 03 00 00 00 02 [CRC16]  (Read Holding Registers, reg 0-1)
 *   응답: 01 03 04 [습도hi lo] [온도hi lo] [CRC16]  (9 bytes)
 *   변환: humidity = raw / 10.0 (%), temperature = raw / 10.0 (C)
 *
 * RS485: uart0 @ 9600 8N1
 *   DE=P1.04, RE#=P1.03, 12V_EN=P0.17
 *
 * 참고: link.h, RS485_PROTOCOL.md
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>

/* ── Pin definitions ── */
#define PIN_12V_EN_PORT0     17  /* P0.17 */
#define PIN_RS485_DE_PORT1    4  /* P1.04 */
#define PIN_RS485_RE_PORT1    3  /* P1.03 */

/* ── Modbus parameters ── */
#define SOIL_SLAVE_ID        0x01  /* 토양센서 */
#define MODBUS_FC_READ_HOLD  0x03
#define POLL_INTERVAL_MS     3000
#define RX_TIMEOUT_MS        500
#define RX_BUF_SZ            32

/* ── Devices ── */
static const struct device *uart_rs485;
static const struct device *gpio0_dev;
static const struct device *gpio1_dev;

/* ── RX buffer (ISR fills) ── */
static uint8_t rx_buf[RX_BUF_SZ];
static volatile uint16_t rx_pos;
static volatile int64_t last_rx_time;

/* ──────────────────────────────────────────────
 *  CRC16-Modbus (poly 0xA001, init 0xFFFF)
 * ────────────────────────────────────────────── */
static uint16_t crc16_modbus(const uint8_t *data, uint16_t len)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x0001) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

/* ──────────────────────────────────────────────
 *  RS485 direction control
 * ────────────────────────────────────────────── */
static void rs485_tx_mode(void)
{
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_RE_PORT1, 1);  /* RE# HIGH (RX off) */
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 1);  /* DE  HIGH (TX on)  */
	k_busy_wait(100);
}

static void rs485_rx_mode(void)
{
	/* 9600 baud: 1 byte ~ 1.04ms, shift register flush 대기 */
	k_busy_wait(2000);
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 0);  /* DE  LOW (TX off) */
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_RE_PORT1, 0);  /* RE# LOW (RX on)  */
}

/* ──────────────────────────────────────────────
 *  UART ISR
 * ────────────────────────────────────────────── */
static void uart_isr(const struct device *dev, void *user_data)
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

/* ──────────────────────────────────────────────
 *  Send raw bytes over RS485
 * ────────────────────────────────────────────── */
static void rs485_send(const uint8_t *data, uint16_t len)
{
	rs485_tx_mode();
	for (uint16_t i = 0; i < len; i++) {
		uart_poll_out(uart_rs485, data[i]);
	}
	rs485_rx_mode();
}

/* ──────────────────────────────────────────────
 *  Wait for RX frame (idle gap detection)
 * ────────────────────────────────────────────── */
static int rs485_wait_rx(uint16_t timeout_ms)
{
	int64_t start = k_uptime_get();

	while ((k_uptime_get() - start) < timeout_ms) {
		k_msleep(5);
		/* 바이트 수신 후 5ms idle → 프레임 완료 */
		if (rx_pos > 0 && (k_uptime_get() - last_rx_time) >= 5) {
			return rx_pos;
		}
	}
	return rx_pos;
}

/* ──────────────────────────────────────────────
 *  Build & send Modbus read request
 * ────────────────────────────────────────────── */
static void modbus_read_holding(uint8_t slave_id, uint16_t addr, uint16_t count)
{
	uint8_t frame[8];

	frame[0] = slave_id;
	frame[1] = MODBUS_FC_READ_HOLD;
	frame[2] = (addr >> 8) & 0xFF;
	frame[3] = addr & 0xFF;
	frame[4] = (count >> 8) & 0xFF;
	frame[5] = count & 0xFF;

	uint16_t crc = crc16_modbus(frame, 6);
	frame[6] = crc & 0xFF;        /* CRC lo */
	frame[7] = (crc >> 8) & 0xFF; /* CRC hi */

	/* RX 버퍼 초기화 */
	uart_irq_rx_disable(uart_rs485);
	rx_pos = 0;
	uart_irq_rx_enable(uart_rs485);

	/* 요청 전송 */
	printk("[SOIL] TX:");
	for (int i = 0; i < 8; i++) {
		printk(" %02X", frame[i]);
	}
	printk("\n");

	rs485_send(frame, 8);
}

/* ──────────────────────────────────────────────
 *  Parse soil sensor response
 * ────────────────────────────────────────────── */
static void parse_soil_response(void)
{
	uint16_t n = rx_pos;

	printk("[SOIL] RX (%u bytes):", n);
	for (uint16_t i = 0; i < n; i++) {
		printk(" %02X", rx_buf[i]);
	}
	printk("\n");

	if (n < 9) {
		printk("[SOIL] ERROR: too short (expected 9)\n");
		return;
	}

	/* 기본 검증 */
	if (rx_buf[0] != SOIL_SLAVE_ID) {
		printk("[SOIL] ERROR: slave_id mismatch (0x%02X)\n", rx_buf[0]);
		return;
	}
	if (rx_buf[1] != MODBUS_FC_READ_HOLD) {
		printk("[SOIL] ERROR: function code 0x%02X (exception?)\n", rx_buf[1]);
		return;
	}
	if (rx_buf[2] != 0x04) {
		printk("[SOIL] ERROR: byte count %u (expected 4)\n", rx_buf[2]);
		return;
	}

	/* CRC 검증 */
	uint16_t crc_calc = crc16_modbus(rx_buf, 7);
	uint16_t crc_recv = rx_buf[7] | (rx_buf[8] << 8);
	if (crc_calc != crc_recv) {
		printk("[SOIL] ERROR: CRC mismatch (calc=0x%04X recv=0x%04X)\n",
		       crc_calc, crc_recv);
		return;
	}

	/* 데이터 추출 */
	int16_t raw_humi = (int16_t)((rx_buf[3] << 8) | rx_buf[4]);
	int16_t raw_temp = (int16_t)((rx_buf[5] << 8) | rx_buf[6]);

	int humi_int = raw_humi / 10;
	int humi_dec = (raw_humi >= 0 ? raw_humi : -raw_humi) % 10;
	int temp_int = raw_temp / 10;
	int temp_dec = (raw_temp >= 0 ? raw_temp : -raw_temp) % 10;

	printk("[SOIL] Humidity: %d.%d %%  Temperature: %d.%d C\n",
	       humi_int, humi_dec, temp_int, temp_dec);
}

/* ──────────────────────────────────────────────
 *  main
 * ────────────────────────────────────────────── */
int main(void)
{
	uint32_t poll_count = 0;

	printk("\n========================================\n");
	printk("  Soil sensor Modbus test - Node (Link)\n");
	printk("  Soil sensor, Slave 0x01, FC 0x03, Reg 0-1\n");
	printk("  uart0 @ 9600, DE=P1.04 RE#=P1.03\n");
	printk("  12V_EN=P0.17, poll %d ms\n", POLL_INTERVAL_MS);
	printk("========================================\n");

	/* GPIO0 (12V_EN) */
	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		printk("[SOIL] gpio0 NOT ready\n");
		return -1;
	}
	gpio_pin_configure(gpio0_dev, PIN_12V_EN_PORT0, GPIO_OUTPUT_LOW);

	/* GPIO1 (DE, RE#) */
	gpio1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1_dev)) {
		printk("[SOIL] gpio1 NOT ready\n");
		return -1;
	}
	gpio_pin_configure(gpio1_dev, PIN_RS485_DE_PORT1, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1_dev, PIN_RS485_RE_PORT1, GPIO_OUTPUT_LOW);

	/* 12V ON (RS485 트랜시버 + 센서 전원) */
	gpio_pin_set_raw(gpio0_dev, PIN_12V_EN_PORT0, 1);
	printk("[SOIL] 12V_EN = HIGH\n");
	k_msleep(500);  /* 센서 부팅 대기 */

	/* RS485 UART */
	uart_rs485 = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart_rs485)) {
		printk("[SOIL] uart0 NOT ready\n");
		return -1;
	}
	uart_irq_callback_set(uart_rs485, uart_isr);
	uart_irq_rx_enable(uart_rs485);
	rs485_rx_mode();
	printk("[SOIL] uart0 ready, RX mode\n");

	/* ── Loopback 진단: DE+RE# 동시 활성으로 자기 TX를 RX로 확인 ── */
	printk("\n[DIAG] loopback test (DE=HIGH, RE#=LOW)...\n");
	uart_irq_rx_disable(uart_rs485);
	rx_pos = 0;
	uart_irq_rx_enable(uart_rs485);

	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 1);  /* TX on */
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_RE_PORT1, 0);  /* RX on */
	k_busy_wait(100);

	uint8_t test_bytes[] = {0xAA, 0x55, 0x01};
	for (int i = 0; i < 3; i++) {
		uart_poll_out(uart_rs485, test_bytes[i]);
	}
	k_busy_wait(2000);
	gpio_pin_set_raw(gpio1_dev, PIN_RS485_DE_PORT1, 0);
	k_msleep(20);

	printk("[DIAG] sent 3 bytes, received %u:", rx_pos);
	for (uint16_t i = 0; i < rx_pos; i++) {
		printk(" %02X", rx_buf[i]);
	}
	printk("\n");
	if (rx_pos >= 3) {
		printk("[DIAG] loopback OK — TX/RX path working\n");
	} else {
		printk("[DIAG] loopback FAIL — check RS485 wiring\n");
	}

	rs485_rx_mode();
	k_msleep(500);

	/* ── Slave ID 스캔 (0x01~0x10) ── */
	printk("\n[SCAN] scanning slave IDs 0x01-0x10 @ 9600...\n");
	for (uint8_t id = 0x01; id <= 0x10; id++) {
		modbus_read_holding(id, 0x0000, 0x0002);
		int rx_len = rs485_wait_rx(300);
		if (rx_len > 0) {
			printk("[SCAN] slave 0x%02X responded! (%d bytes):", id, rx_len);
			for (int i = 0; i < rx_len; i++) {
				printk(" %02X", rx_buf[i]);
			}
			printk("\n");
		}
		k_msleep(100);
	}
	printk("[SCAN] done\n");

	/* ── 일반 polling 루프 ── */
	while (1) {
		poll_count++;
		printk("\n--- poll #%u ---\n", poll_count);

		/* Modbus 요청: slave 0x01, reg 0x0000, count 2 */
		modbus_read_holding(SOIL_SLAVE_ID, 0x0000, 0x0002);

		/* 응답 대기 */
		int rx_len = rs485_wait_rx(RX_TIMEOUT_MS);
		if (rx_len > 0) {
			parse_soil_response();
		} else {
			printk("[SOIL] NO RESPONSE (timeout %d ms)\n", RX_TIMEOUT_MS);
		}

		k_msleep(POLL_INTERVAL_MS);
	}

	return 0;
}
