/*
 * Host test - Tower
 *
 * USB CDC에서 JSON 명령 수신 → debug UART(ttyUSB1)로 출력 + USB CDC로 응답
 *
 * 프로토콜 (text line, '\n' terminated):
 *   Host → Tower:
 *     CMD:LED_ON
 *     CMD:LED_OFF
 *     CMD:BUZZER_ON
 *     CMD:BUZZER_OFF
 *     CMD:SENSOR_READ
 *     CMD:STATUS
 *     DATA:<any text>
 *
 *   Tower → Host:
 *     ACK:<command>
 *     SENSOR:temp=22.5,humi=55.0
 *     STATUS:uptime=<sec>,rx_count=<n>
 *
 * Console: uart0 (ttyUSB1 @ 115200)
 * USB CDC: ttyACMx
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

#define RX_BUF_SZ  256

static const struct device *cdc_dev;
static uint8_t rx_buf[RX_BUF_SZ];
static volatile uint16_t rx_pos;
static uint32_t rx_count;

static void cdc_send(const char *s)
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
				if (rx_pos < RX_BUF_SZ - 1) {
					rx_buf[rx_pos++] = buf[i];
				}
			}
		}
	}
}

static void process_command(char *line)
{
	rx_count++;
	char resp[128];

	printk("[HOST-RX] #%u: \"%s\"\n", rx_count, line);

	if (strcmp(line, "CMD:LED_ON") == 0) {
		printk("[TOWER] LED ON\n");
		cdc_send("ACK:LED_ON\n");

	} else if (strcmp(line, "CMD:LED_OFF") == 0) {
		printk("[TOWER] LED OFF\n");
		cdc_send("ACK:LED_OFF\n");

	} else if (strcmp(line, "CMD:BUZZER_ON") == 0) {
		printk("[TOWER] BUZZER ON\n");
		cdc_send("ACK:BUZZER_ON\n");

	} else if (strcmp(line, "CMD:BUZZER_OFF") == 0) {
		printk("[TOWER] BUZZER OFF\n");
		cdc_send("ACK:BUZZER_OFF\n");

	} else if (strcmp(line, "CMD:SENSOR_READ") == 0) {
		printk("[TOWER] Sensor read (simulated)\n");
		cdc_send("SENSOR:temp=22.5,humi=55.0\n");

	} else if (strcmp(line, "CMD:STATUS") == 0) {
		uint32_t uptime = k_uptime_get_32() / 1000;
		snprintf(resp, sizeof(resp),
			 "STATUS:uptime=%u,rx_count=%u\n", uptime, rx_count);
		printk("[TOWER] %s", resp);
		cdc_send(resp);

	} else if (strncmp(line, "DATA:", 5) == 0) {
		printk("[TOWER] Data received: \"%s\"\n", line + 5);
		snprintf(resp, sizeof(resp), "ACK:DATA_OK len=%u\n",
			 (unsigned)(strlen(line) - 5));
		cdc_send(resp);

	} else {
		printk("[TOWER] Unknown command\n");
		cdc_send("ERR:UNKNOWN\n");
	}
}

int main(void)
{
	printk("\n========================================\n");
	printk("  Host test - Tower\n");
	printk("  Debug: uart0 (ttyUSB1 @ 115200)\n");
	printk("  USB CDC: ttyACMx (host web)\n");
	printk("========================================\n");

	cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(cdc_dev)) {
		printk("[HOST] CDC device not ready\n");
		return -1;
	}
	printk("[HOST] CDC device ready, waiting for host...\n");

	uart_irq_callback_set(cdc_dev, cdc_irq_handler);
	uart_irq_rx_enable(cdc_dev);

	uint32_t tick = 0;

	while (1) {
		/* '\n'이 포함된 라인이 있으면 처리 */
		if (rx_pos > 0) {
			/* rx_buf에서 '\n' 검색 */
			uint16_t n = rx_pos;
			for (uint16_t i = 0; i < n; i++) {
				if (rx_buf[i] == '\n' || rx_buf[i] == '\r') {
					/* 라인 추출 */
					rx_buf[i] = '\0';
					if (i > 0) {
						process_command((char *)rx_buf);
					}
					/* 나머지 데이터를 앞으로 이동 */
					uint16_t remain = n - i - 1;
					if (remain > 0) {
						memmove(rx_buf, &rx_buf[i + 1], remain);
					}
					uart_irq_rx_disable(cdc_dev);
					rx_pos = remain;
					uart_irq_rx_enable(cdc_dev);
					break;
				}
			}
		}

		tick++;
		if (tick % 100 == 0) {
			printk("[HOST] alive tick=%u rx_count=%u\n",
			       tick / 100, rx_count);
		}

		k_msleep(10);
	}

	return 0;
}
