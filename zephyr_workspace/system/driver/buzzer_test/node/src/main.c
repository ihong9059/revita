/*
 * Buzzer basic test - Node (Link)
 *
 * 동작:
 *   1. 12V_EN (P0.17) HIGH → 12V 부스트 ON
 *   2. 2초 간격으로 BUZZER_EN (P0.24) 짧게 ON (100ms beep)
 *   3. 5번 beep (총 10초) 후 BUZZER_EN / 12V_EN 모두 OFF
 *   4. 이후 idle (1초마다 상태 출력만)
 *
 * 참고: link.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define PIN_12V_EN      17  /* P0.17 */
#define PIN_BUZZER_EN   24  /* P0.24 */

#define BEEP_INTERVAL_MS  2000   /* 2초 간격 */
#define BEEP_DURATION_MS  100    /* beep 1회 길이 */
#define BEEP_COUNT        5      /* 5회 → 총 10초 */

int main(void)
{
	const struct device *gpio0;

	printk("\n========================================\n");
	printk("  Buzzer test - Node (Link)\n");
	printk("  12V_EN=P0.17  BUZZER_EN=P0.24\n");
	printk("  pattern: %d ms beep / %d ms interval / %d times\n",
	       BEEP_DURATION_MS, BEEP_INTERVAL_MS, BEEP_COUNT);
	printk("========================================\n");

	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("[BUZZ] gpio0 NOT ready\n");
		return -1;
	}

	gpio_pin_configure(gpio0, PIN_12V_EN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_BUZZER_EN, GPIO_OUTPUT_LOW);

	/* 12V 부스트 ON */
	gpio_pin_set_raw(gpio0, PIN_12V_EN, 1);
	printk("[BUZZ] 12V_EN = HIGH (boost ON)\n");
	k_msleep(50);

	/* Beep 루프 */
	for (int i = 1; i <= BEEP_COUNT; i++) {
		printk("[BUZZ] beep #%d ON\n", i);
		gpio_pin_set_raw(gpio0, PIN_BUZZER_EN, 1);
		k_msleep(BEEP_DURATION_MS);

		gpio_pin_set_raw(gpio0, PIN_BUZZER_EN, 0);
		printk("[BUZZ] beep #%d OFF\n", i);

		/* 다음 beep 까지 대기 (interval - duration) */
		if (i < BEEP_COUNT) {
			k_msleep(BEEP_INTERVAL_MS - BEEP_DURATION_MS);
		}
	}

	/* 종료: 부저 + 12V 모두 OFF */
	gpio_pin_set_raw(gpio0, PIN_BUZZER_EN, 0);
	gpio_pin_set_raw(gpio0, PIN_12V_EN, 0);
	printk("\n[BUZZ] DONE — BUZZER_EN=LOW, 12V_EN=LOW\n");

	/* idle */
	uint32_t tick = 0;
	while (1) {
		k_sleep(K_SECONDS(1));
		tick++;
		printk("[BUZZ] idle tick=%u\n", tick);
	}

	return 0;
}
