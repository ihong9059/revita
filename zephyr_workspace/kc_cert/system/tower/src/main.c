/*
 * System Test - Tower (TX → RX loopback)
 *
 * 동작 (단일 스레드, 순차 처리):
 *   1. LoRa TX: 카운트 메시지 송신
 *   2. LoRa RX: Link에서 loopback된 메시지 수신 대기 (5초 timeout)
 *   3. 수신 결과를 uart1 (debug)과 RS485 (uart0)로 출력
 *   4. 반복
 *
 * RS485: MCP23017 → TMUX1574 MUX 활성화 후 uart0 사용
 * LoRa:  922.1 MHz, SF9, BW125, CR4/6
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/lora.h>
#include <string.h>
#include <stdio.h>

/* ── LoRa parameters ── */
#define LORA_FREQ       922100000
#define LORA_SF         12
#define LORA_BW         BW_125_KHZ
#define LORA_CR         CR_4_6
#define LORA_POWER      14
#define LORA_PREAMBLE   8
#define LORA_PAYLOAD    32

#define RX_TIMEOUT_MS   5000

/* ── MCP23017 ── */
#define REG_IODIRA   0x00
#define REG_OLATA    0x14
#define GPA3_MUX_SEL (1 << 3)
#define GPA4_MUX_EN  (1 << 4)

/* ── Devices ── */
static const struct device *lora_dev;
static const struct device *gpio0_dev;
static const struct device *i2c_dev;
static const struct device *uart0_dev;  /* RS485 */
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
	gpa &= ~GPA3_MUX_SEL;  /* SEL=LOW → RS485 */
	gpa &= ~GPA4_MUX_EN;   /* EN#=LOW → Enable */
	return mcp_write(REG_OLATA, gpa);
}

/* ── RS485 direction control ── */
static void rs485_tx_mode(void)
{
	gpio_pin_set(gpio0_dev, 21, 1);  /* RE#=HIGH */
	gpio_pin_set(gpio0_dev, 17, 1);  /* DE=HIGH */
	k_usleep(100);
}

static void rs485_rx_mode(void)
{
	k_usleep(500);
	gpio_pin_set(gpio0_dev, 17, 0);  /* DE=LOW */
	gpio_pin_set(gpio0_dev, 21, 0);  /* RE#=LOW */
}

static void rs485_send(const char *str)
{
	rs485_tx_mode();
	while (*str) {
		uart_poll_out(uart0_dev, *str++);
	}
	rs485_rx_mode();
}

/* ── Dual output: printk (uart1) + RS485 (uart0) ── */
static void dual_print(const char *msg)
{
	printk("%s", msg);
	rs485_send(msg);
}

/* ── LoRa config ── */
static int lora_configure(bool tx)
{
	struct lora_modem_config config = {
		.frequency = LORA_FREQ,
		.bandwidth = LORA_BW,
		.datarate = LORA_SF,
		.coding_rate = LORA_CR,
		.preamble_len = LORA_PREAMBLE,
		.tx_power = LORA_POWER,
		.tx = tx,
		.iq_inverted = false,
		.public_network = false,
	};
	return lora_config(lora_dev, &config);
}

/* ── Main ── */
int main(void)
{
	char msg[128];

	/* GPIO init */
	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		printk("GPIO0 not ready\n");
		return -1;
	}
	gpio_pin_configure(gpio0_dev, 17, GPIO_OUTPUT_LOW);  /* DE */
	gpio_pin_configure(gpio0_dev, 21, GPIO_OUTPUT_LOW);  /* RE# */

	/* UART0 (RS485) */
	uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart0_dev)) {
		printk("UART0 not ready\n");
		return -1;
	}

	printk("\n========================================\n");
	printk("  System Test - Tower v1.0\n");
	printk("  LoRa TX -> Link -> Loopback -> RX\n");
	printk("========================================\n");

	/* I2C → MCP23017 → MUX enable */
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (device_is_ready(i2c_dev)) {
		k_msleep(100);
		for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
			uint8_t reg = REG_IODIRA, val;
			if (i2c_write_read(i2c_dev, addr, &reg, 1, &val, 1) == 0) {
				mcp_addr = addr;
				break;
			}
		}
		if (mcp_addr) {
			mux_enable_rs485();
			printk("[MUX] RS485 enabled (MCP@0x%02X)\n", mcp_addr);
		} else {
			printk("[MUX] MCP23017 not found\n");
		}
	}

	rs485_send("\r\n=== Tower System Test v1.0 ===\r\n");

	/* LoRa init */
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		dual_print("[LoRa] Device not ready!\r\n");
		return -1;
	}
	dual_print("[LoRa] SX1262 ready (922.1 MHz, SF9)\r\n");

	/* TX → RX loop (단일 스레드) */
	uint32_t count = 0;
	uint32_t rx_ok = 0, rx_fail = 0;

	while (1) {
		/* === TX === */
		if (lora_configure(true) < 0) {
			dual_print("[Tower-TX] Config failed\r\n");
			k_sleep(K_SECONDS(3));
			continue;
		}

		uint8_t payload[LORA_PAYLOAD];
		memset(payload, 0, LORA_PAYLOAD);
		int plen = snprintf((char *)payload, LORA_PAYLOAD,
				    "TOWER:%u", count);

		snprintf(msg, sizeof(msg),
			 "[Tower-TX] #%u \"%s\" sending...\r\n", count, payload);
		dual_print(msg);

		int ret = lora_send(lora_dev, payload, plen);
		if (ret < 0) {
			snprintf(msg, sizeof(msg),
				 "[Tower-TX] Send failed: %d\r\n", ret);
			dual_print(msg);
			k_sleep(K_SECONDS(3));
			continue;
		}

		snprintf(msg, sizeof(msg),
			 "[Tower-TX] Sent OK (%d bytes)\r\n", plen);
		dual_print(msg);
		count++;

		/* === RX (loopback 수신 대기) === */
		if (lora_configure(false) < 0) {
			dual_print("[Tower-RX] Config failed\r\n");
			k_sleep(K_SECONDS(3));
			continue;
		}

		uint8_t rx_buf[LORA_PAYLOAD];
		int16_t rssi;
		int8_t snr;

		ret = lora_recv(lora_dev, rx_buf, LORA_PAYLOAD,
				K_MSEC(RX_TIMEOUT_MS), &rssi, &snr);

		if (ret < 0) {
			rx_fail++;
			snprintf(msg, sizeof(msg),
				 "[Tower-RX] Timeout (no loopback) [ok=%u fail=%u]\r\n",
				 rx_ok, rx_fail);
			dual_print(msg);
		} else {
			rx_ok++;
			rx_buf[ret < LORA_PAYLOAD ? ret : LORA_PAYLOAD - 1] = '\0';
			snprintf(msg, sizeof(msg),
				 "[Tower-RX] Loopback OK! \"%s\" RSSI=%d SNR=%d [ok=%u fail=%u]\r\n",
				 rx_buf, rssi, snr, rx_ok, rx_fail);
			dual_print(msg);
		}

		/* 다음 TX까지 1초 대기 */
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
