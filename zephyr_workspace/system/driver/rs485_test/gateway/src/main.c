/*
 * RS485 basic test - Gateway (Tower)
 *
 * kc_cert/tower 검증된 패턴 이식.
 *
 * 매핑:
 *   - uart0 = console + RS485 wire @ 115200 8N1 (P0.20 TX / P0.19 RX)
 *   - DE  = P0.17 (RAK_DE,  MCU 직접 GPIO)
 *   - RE# = P0.21 (RAK_RE#, MCU 직접 GPIO)
 *   - MCP23017 @ 0x20:
 *       GPA3 = MUX_SEL  → LOW (RS485 채널)
 *       GPA4 = MUX_EN#  → LOW (enable)
 *       그 외 power enable 비트는 건드리지 않음
 *
 * 초기화 순서 (중요):
 *   1) gpio0 init (DE/RE# 출력 LOW)
 *   2) MCP23017 probe + MUX 활성       ← 먼저!
 *   3) RS485 wire 에 부팅 메시지 송신
 *   4) 메인 루프: 2초마다 "Tower:N\r\n"
 *
 * 동작 검증:
 *   - ttyUSB1 (P0.20 직결 USB-UART, 115200) 에서 디버그 + wire 출력 모두 보임
 *   - RS485 A/B 라인을 USB-RS485 어댑터(115200 8N1)로 모니터하면 wire 출력 확인
 *
 * 주의:
 *   - 2026-04-07 시점 wire 출력 미확인 (HW 의심) — 디버그 콘솔(ttyUSB1)
 *     출력은 정상이지만 RS485 A/B 라인에 신호가 도달하지 않음.
 *     트랜시버 전원 / MUX 라우팅 / 외부 termination 등 보드 검증 필요.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* MCU direct pins (P0.x — port-relative) */
#define PIN_RS485_DE   17  /* P0.17 */
#define PIN_RS485_RE   21  /* P0.21 */

/* MCP23017 */
#define MCP_REG_IODIRA  0x00
#define MCP_REG_OLATA   0x14

#define GPA0_LTE_RST_N  (1U << 0)
#define GPA2_12_14V_EN  (1U << 2)
#define GPA3_MUX_SEL    (1U << 3)
#define GPA4_MUX_EN_N   (1U << 4)
#define GPA5_3V3_EN     (1U << 5)
#define GPA6_5V_EN      (1U << 6)

static const struct device *uart_dev;
static const struct device *gpio0_dev;
static const struct device *i2c_dev;
static uint8_t mcp_addr;

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

	if (mcp_read(MCP_REG_IODIRA, &iodira)) return -1;
	iodira &= ~(GPA0_LTE_RST_N | GPA2_12_14V_EN |
		    GPA3_MUX_SEL | GPA4_MUX_EN_N |
		    GPA5_3V3_EN | GPA6_5V_EN);
	if (mcp_write(MCP_REG_IODIRA, iodira)) return -1;

	if (mcp_read(MCP_REG_OLATA, &gpa)) return -1;
	gpa |= GPA0_LTE_RST_N;     /* LTE reset 해제 */
	gpa |= GPA5_3V3_EN;        /* 3.3V_SW ON — TMUX1574 / 트랜시버 VDD */
	gpa |= GPA6_5V_EN;         /* 5V ON */
	gpa |= GPA2_12_14V_EN;     /* 12V 부스트 ON (RS485 PORT) */
	gpa &= ~GPA3_MUX_SEL;      /* SEL = LOW → RS485 채널 */
	gpa &= ~GPA4_MUX_EN_N;     /* EN# = LOW → MUX enable */
	if (mcp_write(MCP_REG_OLATA, gpa)) return -1;
	k_msleep(50);              /* power 안정화 */
	return 0;
}

static void rs485_tx_mode(void)
{
	gpio_pin_set_raw(gpio0_dev, PIN_RS485_RE, 1);   /* RE#=HIGH: RX off */
	gpio_pin_set_raw(gpio0_dev, PIN_RS485_DE, 1);   /* DE=HIGH:  TX on  */
	k_usleep(100);
}

static void rs485_rx_mode(void)
{
	k_usleep(2000);                                  /* shift register flush */
	gpio_pin_set_raw(gpio0_dev, PIN_RS485_DE, 0);   /* DE=LOW:  TX off */
	gpio_pin_set_raw(gpio0_dev, PIN_RS485_RE, 0);   /* RE#=LOW: RX on  */
}

static void rs485_send(const char *str)
{
	while (*str) {
		uart_poll_out(uart_dev, *str++);
	}
}

int main(void)
{
	uint32_t count = 0;
	char buf[64];

	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		return -1;
	}
	gpio_pin_configure(gpio0_dev, PIN_RS485_DE, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0_dev, PIN_RS485_RE, GPIO_OUTPUT_LOW);

	uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
	if (!device_is_ready(uart_dev)) {
		return -1;
	}

	/* === 1단계: MCP23017 초기화 + MUX RS485 채널 활성 (먼저!) === */
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (device_is_ready(i2c_dev)) {
		k_msleep(100);
		for (uint8_t a = 0x20; a <= 0x27; a++) {
			uint8_t reg = MCP_REG_IODIRA, val;
			if (i2c_write_read(i2c_dev, a, &reg, 1, &val, 1) == 0) {
				mcp_addr = a;
				break;
			}
		}
		if (mcp_addr) {
			mux_enable_rs485();
			k_msleep(10);  /* MUX 안정화 */
			printk("[GW] MCP23017 @ 0x%02X, MUX SEL=LOW EN#=LOW (RS485 channel)\n",
			       mcp_addr);
		} else {
			printk("[GW] MCP23017 not found — MUX may not route to RS485!\n");
		}
	} else {
		printk("[GW] I2C not ready\n");
	}

	/* === 2단계: 이제 RS485 wire 에 부팅 메시지 송신 === */
	rs485_tx_mode();
	rs485_send("\r\n=== Tower RS485 test ===\r\n");
	if (mcp_addr) {
		snprintf(buf, sizeof(buf),
			 "MCP @ 0x%02X, MUX = RS485\r\n", mcp_addr);
		rs485_send(buf);
	}
	rs485_send("--- start ---\r\n");

	while (1) {
		int len = snprintf(buf, sizeof(buf), "Tower:%u\r\n", count++);
		(void)len;
		rs485_send(buf);
		k_msleep(2000);
	}

	return 0;
}
