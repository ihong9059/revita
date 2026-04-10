/*
 * Motor driver test - Node (Link) Y channel
 *
 * Y_EN_A  = P0.25 (HIGH=CW component)
 * Y_EN_B  = P1.01 (HIGH=CCW component)
 * Y_EN_P2 = P1.02 (반드시 LOW 유지)
 * 12V_EN  = P0.17 (motor power)
 *
 * CW:   EN_A=HIGH, EN_B=LOW
 * STOP: EN_A=LOW,  EN_B=LOW
 * CCW:  EN_A=LOW,  EN_B=HIGH
 *
 * 패턴: 3s CW → 1s STOP → 3s CCW → 1s STOP × 3회 → 정지
 *
 * 참고: link.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* P0 pins */
#define PIN_Y_EN_A    25  /* P0.25 */
#define PIN_12V_EN    17  /* P0.17 */

/* P1 pins */
#define PIN_Y_EN_B     1  /* P1.01 */
#define PIN_Y_EN_P2    2  /* P1.02 */

#define CW_TIME_MS   3000
#define CCW_TIME_MS  3000
#define STOP_TIME_MS 1000
#define REPEAT_COUNT 3

static const struct device *gpio0;
static const struct device *gpio1;

static void motor_cw(void)
{
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 1);
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
	printk("[MOTOR-Y] CW\n");
}

static void motor_ccw(void)
{
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 1);
	printk("[MOTOR-Y] CCW\n");
}

static void motor_stop(void)
{
	gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
	gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
	printk("[MOTOR-Y] STOP\n");
}

int main(void)
{
	printk("\n========================================\n");
	printk("  Motor driver test - Node Y channel\n");
	printk("  Y_EN_A=P0.25  Y_EN_B=P1.01\n");
	printk("  Y_EN_P2=P1.02 (LOW fixed)  12V_EN=P0.17\n");
	printk("  pattern: 3s CW / 1s STOP / 3s CCW\n");
	printk("  repeat: %d times\n", REPEAT_COUNT);
	printk("========================================\n");

	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("[MOTOR-Y] gpio0 NOT ready\n");
		return -1;
	}

	gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1)) {
		printk("[MOTOR-Y] gpio1 NOT ready\n");
		return -1;
	}

	gpio_pin_configure(gpio0, PIN_12V_EN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_Y_EN_A, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_Y_EN_B, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_Y_EN_P2, GPIO_OUTPUT_LOW);

	/* 12V ON */
	gpio_pin_set_raw(gpio0, PIN_12V_EN, 1);
	printk("[MOTOR-Y] 12V_EN = HIGH\n");
	k_msleep(100);

	/* EN_P2 반드시 LOW 유지 */
	gpio_pin_set_raw(gpio1, PIN_Y_EN_P2, 0);
	printk("[MOTOR-Y] Y_EN_P2 = LOW (fixed)\n");
	k_msleep(50);

	for (int i = 1; i <= REPEAT_COUNT; i++) {
		printk("\n--- cycle %d/%d ---\n", i, REPEAT_COUNT);

		motor_cw();
		k_msleep(CW_TIME_MS);

		motor_stop();
		k_msleep(STOP_TIME_MS);

		motor_ccw();
		k_msleep(CCW_TIME_MS);

		motor_stop();
		if (i < REPEAT_COUNT) {
			k_msleep(STOP_TIME_MS);
		}
	}

	/* 종료 */
	gpio_pin_set_raw(gpio0, PIN_12V_EN, 0);
	printk("\n[MOTOR-Y] DONE — all OFF\n");

	uint32_t tick = 0;
	while (1) {
		k_sleep(K_SECONDS(1));
		tick++;
		printk("[MOTOR-Y] idle tick=%u\n", tick);
	}

	return 0;
}
