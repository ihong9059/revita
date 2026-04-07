/*
 * REVITA Tower - debug skeleton
 * 1초마다 카운터 값을 RAK_UART1(P0.20)으로 출력
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	uint32_t counter = 0;

	printk("\n=== REVITA Tower boot ===\n");

	while (1) {
		counter++;
		printk("tower: %u\n", counter);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
