/*
 * System LTE - Tower Board
 *
 * RC76xx LTE 모듈 AT command 제어
 *
 * UART0 (P0.20/P0.19): Debug console (RS485 경로)
 * UART1 (P0.16/P0.15): LTE module (RAK_UART2)
 *
 * MCP23017 GPA0: LTE_RST_N (Active Low)
 *
 * 동작:
 *   1. MCP23017 초기화 → LTE 모듈 리셋 해제
 *   2. AT command 순차 실행 → 모듈 상태 확인
 *   3. 네트워크 등록 대기
 *   4. 주기적으로 신호 품질 확인
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* ── Port Defines (tower_port.h 기반) ── */

/* MCP23017 Registers */
#define MCP_REG_IODIRA          0x00
#define MCP_REG_IODIRB          0x01
#define MCP_REG_OLATA           0x14
#define MCP_REG_OLATB           0x15

/* MCP23017 GPA bits */
#define MCP_GPA0_LTE_RST_N     (1 << 0)    /* LTE 리셋 (Active Low) */
#define MCP_GPA3_MUX_SEL        (1 << 3)    /* TMUX1574 SEL */
#define MCP_GPA4_MUX_EN_N       (1 << 4)    /* TMUX1574 EN# */

/* RS485 Direction Control */
#define PIN_RS485_DE            17      /* P0.17 */
#define PIN_RS485_RE_N          21      /* P0.21 */

/* ── AT Command RX Buffer ── */
#define AT_RX_BUF_SIZE          512
#define AT_LINE_MAX             256

static uint8_t rx_ring[AT_RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

/* ── Devices ── */
static const struct device *gpio0_dev;
static const struct device *i2c_dev;
static const struct device *lte_uart;   /* uart1 - LTE */
static uint8_t mcp_addr;

/* ── UART1 RX ISR (LTE 응답 수신) ── */
static void lte_uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		uint8_t c;
		if (uart_fifo_read(dev, &c, 1) == 1) {
			uint16_t next = (rx_head + 1) % AT_RX_BUF_SIZE;
			if (next != rx_tail) {
				rx_ring[rx_head] = c;
				rx_head = next;
			}
		}
	}
}

/* ring buffer에서 1바이트 읽기 */
static int rx_getc(uint8_t *c)
{
	if (rx_tail == rx_head) {
		return -1;
	}
	*c = rx_ring[rx_tail];
	rx_tail = (rx_tail + 1) % AT_RX_BUF_SIZE;
	return 0;
}

/* ring buffer flush */
static void rx_flush(void)
{
	rx_tail = rx_head;
}

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

/* ── RS485 방향 제어 (debug 출력용) ── */
static void rs485_tx_mode(void)
{
	gpio_pin_set(gpio0_dev, PIN_RS485_RE_N, 1);
	gpio_pin_set(gpio0_dev, PIN_RS485_DE, 1);
	k_usleep(100);
}

static void rs485_rx_mode(void)
{
	k_usleep(500);
	gpio_pin_set(gpio0_dev, PIN_RS485_DE, 0);
	gpio_pin_set(gpio0_dev, PIN_RS485_RE_N, 0);
}

/* ── LTE UART TX (AT command 전송) ── */
static void lte_send(const char *str)
{
	while (*str) {
		uart_poll_out(lte_uart, *str++);
	}
}

static void lte_send_at(const char *cmd)
{
	printk("[LTE-TX] %s\n", cmd);
	rx_flush();
	lte_send(cmd);
	lte_send("\r\n");
}

/* ── AT 응답 수신 (timeout 포함) ── */
/*
 * timeout_ms 동안 응답을 수집하고, "OK" 또는 "ERROR" 를 만나면 즉시 리턴.
 * 반환값: 수신된 바이트 수, resp에 null-terminated 문자열 저장.
 */
static int lte_recv(char *resp, int resp_size, int timeout_ms)
{
	int idx = 0;
	int64_t deadline = k_uptime_get() + timeout_ms;

	memset(resp, 0, resp_size);

	while (k_uptime_get() < deadline) {
		uint8_t c;
		if (rx_getc(&c) == 0) {
			if (idx < resp_size - 1) {
				resp[idx++] = (char)c;
				resp[idx] = '\0';
			}
			/* "OK\r\n" 또는 "ERROR" 감지 시 조기 종료 */
			if (idx >= 4) {
				if (strstr(resp, "\r\nOK\r\n") ||
				    strstr(resp, "\r\nERROR\r\n") ||
				    strstr(resp, "\r\nERROR") ||
				    /* 첫 줄이 OK인 경우 */
				    (idx >= 2 && resp[0] == 'O' && resp[1] == 'K')) {
					k_msleep(10); /* 나머지 데이터 대기 */
					/* 잔여 바이트 수집 */
					while (rx_getc(&c) == 0 && idx < resp_size - 1) {
						resp[idx++] = (char)c;
					}
					resp[idx] = '\0';
					break;
				}
			}
		} else {
			k_msleep(1);
		}
	}

	return idx;
}

/* ── AT command 실행 + 응답 출력 ── */
static int at_cmd(const char *cmd, char *resp, int resp_size, int timeout_ms)
{
	lte_send_at(cmd);
	int len = lte_recv(resp, resp_size, timeout_ms);

	if (len > 0) {
		printk("[LTE-RX] %s\n", resp);
	} else {
		printk("[LTE-RX] (no response, timeout %d ms)\n", timeout_ms);
	}

	return len;
}

/* 응답에 target 문자열이 포함되어 있는지 확인 */
static bool at_resp_contains(const char *resp, const char *target)
{
	return strstr(resp, target) != NULL;
}

/* ── LTE 모듈 리셋 (MCP23017 GPA0) ── */
static int lte_reset(void)
{
	uint8_t iodira, gpa;

	if (!mcp_addr) {
		printk("[LTE] MCP23017 not found, skip reset\n");
		return -1;
	}

	/* GPA0을 출력으로 설정 */
	if (mcp_read(MCP_REG_IODIRA, &iodira)) {
		return -1;
	}
	iodira &= ~MCP_GPA0_LTE_RST_N;
	if (mcp_write(MCP_REG_IODIRA, iodira)) {
		return -1;
	}

	/* RST_N = LOW (리셋 활성화) */
	if (mcp_read(MCP_REG_OLATA, &gpa)) {
		return -1;
	}
	gpa &= ~MCP_GPA0_LTE_RST_N;
	if (mcp_write(MCP_REG_OLATA, gpa)) {
		return -1;
	}
	printk("[LTE] Reset LOW\n");
	k_msleep(500);

	/* RST_N = HIGH (리셋 해제) */
	gpa |= MCP_GPA0_LTE_RST_N;
	if (mcp_write(MCP_REG_OLATA, gpa)) {
		return -1;
	}
	printk("[LTE] Reset HIGH (released)\n");

	/* RC76xx 부팅 대기 (약 3~5초) */
	printk("[LTE] Waiting for module boot...\n");
	k_msleep(5000);

	return 0;
}

/* ── MUX → RS485 경로 활성화 (debug console 출력용) ── */
static int mux_enable_rs485(void)
{
	uint8_t iodira, gpa;

	if (mcp_read(MCP_REG_IODIRA, &iodira)) {
		return -1;
	}
	iodira &= ~(MCP_GPA3_MUX_SEL | MCP_GPA4_MUX_EN_N);
	if (mcp_write(MCP_REG_IODIRA, iodira)) {
		return -1;
	}

	if (mcp_read(MCP_REG_OLATA, &gpa)) {
		return -1;
	}
	gpa &= ~MCP_GPA3_MUX_SEL;     /* SEL=LOW → RS485 */
	gpa &= ~MCP_GPA4_MUX_EN_N;    /* EN#=LOW → Enable */
	return mcp_write(MCP_REG_OLATA, gpa);
}

/* ── Main ── */
int main(void)
{
	char resp[AT_LINE_MAX];

	/* GPIO init */
	gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0_dev)) {
		printk("GPIO0 not ready\n");
		return -1;
	}
	gpio_pin_configure(gpio0_dev, PIN_RS485_DE, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0_dev, PIN_RS485_RE_N, GPIO_OUTPUT_LOW);

	/* LTE UART (uart1 = RAK_UART2, P0.16 TX / P0.15 RX) */
	lte_uart = DEVICE_DT_GET(DT_NODELABEL(uart1));
	if (!device_is_ready(lte_uart)) {
		printk("UART1 (LTE) not ready\n");
		return -1;
	}

	/* UART1 RX interrupt 활성화 */
	uart_irq_callback_set(lte_uart, lte_uart_isr);
	uart_irq_rx_enable(lte_uart);

	printk("\n========================================\n");
	printk("  System LTE - Tower v1.0\n");
	printk("  RC76xx AT Command Interface\n");
	printk("  LTE UART: P0.16(TX) / P0.15(RX)\n");
	printk("========================================\n");

	/* I2C → MCP23017 검색 */
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (device_is_ready(i2c_dev)) {
		k_msleep(100);
		for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
			uint8_t reg = MCP_REG_IODIRA, val;
			if (i2c_write_read(i2c_dev, addr, &reg, 1, &val, 1) == 0) {
				mcp_addr = addr;
				break;
			}
		}
		if (mcp_addr) {
			printk("[MCP] Found at 0x%02X\n", mcp_addr);
			mux_enable_rs485();
			printk("[MUX] RS485 debug path enabled\n");
		} else {
			printk("[MCP] MCP23017 not found\n");
		}
	}

	/* LTE 모듈 리셋 */
	lte_reset();

	/* ── AT command 초기화 시퀀스 ── */
	printk("\n--- AT Command Init ---\n");

	/* Echo off + 기본 확인 */
	at_cmd("ATE0", resp, sizeof(resp), 2000);
	at_cmd("AT", resp, sizeof(resp), 2000);

	/* 모듈 정보 */
	at_cmd("ATI", resp, sizeof(resp), 2000);

	/* SIM 상태 확인 */
	at_cmd("AT+CPIN?", resp, sizeof(resp), 2000);

	/* 네트워크 등록 상태 */
	at_cmd("AT+CREG?", resp, sizeof(resp), 2000);

	/* 신호 품질 */
	at_cmd("AT+CSQ", resp, sizeof(resp), 2000);

	/* GPRS 연결 상태 */
	at_cmd("AT+CGATT?", resp, sizeof(resp), 2000);

	/* 오퍼레이터 정보 */
	at_cmd("AT+COPS?", resp, sizeof(resp), 2000);

	printk("\n--- Init Complete ---\n");

	/* ── 주기적 상태 모니터링 ── */
	uint32_t count = 0;

	while (1) {
		printk("\n[Monitor] #%u\n", count++);

		/* 네트워크 등록 */
		at_cmd("AT+CREG?", resp, sizeof(resp), 2000);

		/* 신호 품질 */
		at_cmd("AT+CSQ", resp, sizeof(resp), 2000);

		/* 15초 대기 */
		k_sleep(K_SECONDS(15));
	}

	return 0;
}
