/*
 * Host test - Tower
 *
 * USB CDC에서 명령 수신 → debug UART(ttyUSB1)로 출력 + USB CDC로 응답
 *
 * 프로토콜 (text line, '\n' terminated):
 *
 *   Host → Tower:
 *     CMD:LED_ON / CMD:LED_OFF
 *     CMD:BUZZER_ON / CMD:BUZZER_OFF
 *     CMD:MOTOR_X_CW / CMD:MOTOR_X_CCW / CMD:MOTOR_X_STOP
 *     CMD:MOTOR_Y_CW / CMD:MOTOR_Y_CCW / CMD:MOTOR_Y_STOP
 *     CMD:SENSOR_READ
 *     CMD:INPUT_READ
 *     CMD:STATUS
 *     DATA:<any text>
 *
 *   Tower → Host:
 *     ACK:<command>
 *     SENSOR:soil1_temp=T,soil1_humi=H,soil2_temp=T,soil2_humi=H,leaf_temp=T,leaf_humi=H
 *     INPUT:btn=0|1,vib=0|1,flow1=N,flow2=N,bat=V
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

/* Simulated node state */
static struct {
	/* output */
	bool led;
	bool buzzer;
	int motor_x;   /* -1=CCW, 0=STOP, 1=CW */
	int motor_y;

	/* input (simulated) */
	bool btn;
	bool vib;
	uint32_t flow1;
	uint32_t flow2;
	float bat;

	/* sensor (simulated) */
	float soil1_temp, soil1_humi;
	float soil2_temp, soil2_humi;
	float leaf_temp, leaf_humi;
} node = {
	.bat = 3.85f,
	.soil1_temp = 22.9f, .soil1_humi = 45.2f,
	.soil2_temp = 22.6f, .soil2_humi = 48.7f,
	.leaf_temp = 22.4f,  .leaf_humi = 62.1f,
};

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

static const char *motor_str(int state)
{
	if (state > 0) return "CW";
	if (state < 0) return "CCW";
	return "STOP";
}

static void process_command(char *line)
{
	rx_count++;
	char resp[256];

	printk("[HOST-RX] #%u: \"%s\"\n", rx_count, line);

	/* --- LED --- */
	if (strcmp(line, "CMD:LED_ON") == 0) {
		node.led = true;
		printk("[NODE] LED ON\n");
		cdc_send("ACK:LED_ON\n");

	} else if (strcmp(line, "CMD:LED_OFF") == 0) {
		node.led = false;
		printk("[NODE] LED OFF\n");
		cdc_send("ACK:LED_OFF\n");

	/* --- Buzzer --- */
	} else if (strcmp(line, "CMD:BUZZER_ON") == 0) {
		node.buzzer = true;
		printk("[NODE] BUZZER ON\n");
		cdc_send("ACK:BUZZER_ON\n");

	} else if (strcmp(line, "CMD:BUZZER_OFF") == 0) {
		node.buzzer = false;
		printk("[NODE] BUZZER OFF\n");
		cdc_send("ACK:BUZZER_OFF\n");

	/* --- Motor X --- */
	} else if (strcmp(line, "CMD:MOTOR_X_CW") == 0) {
		node.motor_x = 1;
		printk("[NODE] Motor X → CW\n");
		cdc_send("ACK:MOTOR_X_CW\n");

	} else if (strcmp(line, "CMD:MOTOR_X_CCW") == 0) {
		node.motor_x = -1;
		printk("[NODE] Motor X → CCW\n");
		cdc_send("ACK:MOTOR_X_CCW\n");

	} else if (strcmp(line, "CMD:MOTOR_X_STOP") == 0) {
		node.motor_x = 0;
		printk("[NODE] Motor X → STOP\n");
		cdc_send("ACK:MOTOR_X_STOP\n");

	/* --- Motor Y --- */
	} else if (strcmp(line, "CMD:MOTOR_Y_CW") == 0) {
		node.motor_y = 1;
		printk("[NODE] Motor Y → CW\n");
		cdc_send("ACK:MOTOR_Y_CW\n");

	} else if (strcmp(line, "CMD:MOTOR_Y_CCW") == 0) {
		node.motor_y = -1;
		printk("[NODE] Motor Y → CCW\n");
		cdc_send("ACK:MOTOR_Y_CCW\n");

	} else if (strcmp(line, "CMD:MOTOR_Y_STOP") == 0) {
		node.motor_y = 0;
		printk("[NODE] Motor Y → STOP\n");
		cdc_send("ACK:MOTOR_Y_STOP\n");

	/* --- Sensor read (RS485 sensors) --- */
	} else if (strcmp(line, "CMD:SENSOR_READ") == 0) {
		printk("[NODE] Sensor read\n");
		snprintf(resp, sizeof(resp),
			"SENSOR:soil1_temp=%.1f,soil1_humi=%.1f,"
			"soil2_temp=%.1f,soil2_humi=%.1f,"
			"leaf_temp=%.1f,leaf_humi=%.1f\n",
			(double)node.soil1_temp, (double)node.soil1_humi,
			(double)node.soil2_temp, (double)node.soil2_humi,
			(double)node.leaf_temp, (double)node.leaf_humi);
		cdc_send(resp);

	/* --- Input read (digital + counter + battery) --- */
	} else if (strcmp(line, "CMD:INPUT_READ") == 0) {
		/* Simulate flow counter increment */
		node.flow1 += 3;
		node.flow2 += 5;
		printk("[NODE] Input read\n");
		snprintf(resp, sizeof(resp),
			"INPUT:btn=%d,vib=%d,flow1=%u,flow2=%u,bat=%.2f\n",
			node.btn ? 1 : 0,
			node.vib ? 1 : 0,
			node.flow1, node.flow2,
			(double)node.bat);
		cdc_send(resp);

	/* --- Status --- */
	} else if (strcmp(line, "CMD:STATUS") == 0) {
		uint32_t uptime = k_uptime_get_32() / 1000;
		snprintf(resp, sizeof(resp),
			"STATUS:uptime=%u,rx_count=%u,led=%d,buzzer=%d,"
			"motor_x=%s,motor_y=%s\n",
			uptime, rx_count,
			node.led ? 1 : 0, node.buzzer ? 1 : 0,
			motor_str(node.motor_x), motor_str(node.motor_y));
		printk("[NODE] %s", resp);
		cdc_send(resp);

	/* --- Data passthrough --- */
	} else if (strncmp(line, "DATA:", 5) == 0) {
		printk("[NODE] Data: \"%s\"\n", line + 5);
		snprintf(resp, sizeof(resp), "ACK:DATA_OK len=%u\n",
			 (unsigned)(strlen(line) - 5));
		cdc_send(resp);

	} else {
		printk("[NODE] Unknown command\n");
		cdc_send("ERR:UNKNOWN\n");
	}
}

int main(void)
{
	printk("\n========================================\n");
	printk("  REVITA Node Monitor - Tower Bridge\n");
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
			uint16_t n = rx_pos;
			for (uint16_t i = 0; i < n; i++) {
				if (rx_buf[i] == '\n' || rx_buf[i] == '\r') {
					rx_buf[i] = '\0';
					if (i > 0) {
						process_command((char *)rx_buf);
					}
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
