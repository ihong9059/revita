#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

/* 12V_EN: P0.17 */
#define PIN_12V_EN     17
/* X축 모터 (gpio0) */
#define PIN_X_EN_A     14  /* P0.14 - CW (개방) */
#define PIN_X_EN_B     13  /* P0.13 - CCW (폐쇄) */
#define PIN_X_EN_P2     4  /* P0.04 - 12V MOSFET */
/* Y축 모터 */
#define PIN_Y_EN_A     25  /* P0.25 - CW (개방) - gpio0 */
#define PIN_Y_EN_B      1  /* P1.01 - CCW (폐쇄) - gpio1 */
#define PIN_Y_EN_P2     2  /* P1.02 - 12V MOSFET - gpio1 */

static const struct device *gpio0;
static const struct device *gpio1;
static const struct device *uart_dev;

/* X축 */
static void motor_x_stop(void)
{
	gpio_pin_set_raw(gpio0, PIN_X_EN_A, 0);
	gpio_pin_set_raw(gpio0, PIN_X_EN_B, 0);
	printk("X Motor STOP (A=LOW, B=LOW)\n");
}

static void motor_x_cw(void)
{
	gpio_pin_set_raw(gpio0, PIN_X_EN_B, 0);
	gpio_pin_set_raw(gpio0, PIN_X_EN_A, 1);
	printk("X Motor CW   (A=HIGH, B=LOW)\n");
}

static void motor_x_ccw(void)
{
	gpio_pin_set_raw(gpio0, PIN_X_EN_A, 0);
	gpio_pin_set_raw(gpio0, PIN_X_EN_B, 1);
	printk("X Motor CCW  (A=LOW, B=HIGH)\n");
}

/* Y축 */
static void motor_y_stop(void)
{
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
	printk("Y Motor STOP (A=LOW, B=LOW)\n");
}

static void motor_y_cw(void)
{
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 1);
	printk("Y Motor CW   (A=HIGH, B=LOW)\n");
}

static void motor_y_ccw(void)
{
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 1);
	printk("Y Motor CCW  (A=LOW, B=HIGH)\n");
}

static void uart_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	while (uart_irq_rx_ready(dev)) {
		if (uart_fifo_read(dev, &c, 1) != 1) {
			continue;
		}

		switch (c) {
		case '0':
			motor_x_stop();
			break;
		case '1':
			motor_x_cw();
			break;
		case '2':
			motor_x_ccw();
			break;
		case '3':
			motor_y_stop();
			break;
		case '4':
			motor_y_cw();
			break;
		case '5':
			motor_y_ccw();
			break;
		default:
			printk("Unknown key: '%c'\n", c);
			printk("  X: 0=STOP 1=CW 2=CCW\n");
			printk("  Y: 3=STOP 4=CW 5=CCW\n");
			break;
		}
	}
}

int main(void)
{
	int ret;

	printk("\n========================================\n");
	printk("  HW Test - X/Y Motor + Power\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  X: 0=STOP  1=CW(개방)  2=CCW(폐쇄)\n");
	printk("  Y: 3=STOP  4=CW(개방)  5=CCW(폐쇄)\n");
	printk("========================================\n\n");

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

	/* 12V_EN - 항상 ON */
	ret = gpio_pin_configure(gpio0, PIN_12V_EN, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		printk("ERROR: 12V_EN config: %d\n", ret);
		return -1;
	}
	printk("12V_EN (P0.17) = HIGH (ON)\n");

	/* X축 핀 초기화 */
	gpio_pin_configure(gpio0, PIN_X_EN_A, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_X_EN_B, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_X_EN_P2, GPIO_OUTPUT_LOW);
	printk("X Motor: A=P0.14, B=P0.13\n");

	/* Y축 핀 초기화 */
	gpio_pin_configure(gpio0, PIN_Y_EN_A, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_Y_EN_B, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_Y_EN_P2, GPIO_OUTPUT_LOW);
	printk("Y Motor: A=P0.25, B=P1.01\n");

	/* UART 인터럽트 */
	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (!device_is_ready(uart_dev)) {
		printk("ERROR: UART not ready\n");
		return -1;
	}

	uart_irq_callback_set(uart_dev, uart_cb);
	uart_irq_rx_enable(uart_dev);

	printk("\nReady. Press 0~5...\n\n");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
