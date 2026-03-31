#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

/* LED_EN: P0.15 */
#define PIN_LED_EN  15

/* 테스트용 오프셋 (마지막 섹터 사용, 데이터 영역에 영향 없음) */
#define TEST_OFFSET  0x1FF000  /* 2MB - 4KB */
#define TEST_SIZE    256
#define PAGE_SIZE    4096

static const struct device *flash_dev;

static int flash_self_test(void)
{
	uint8_t write_buf[TEST_SIZE];
	uint8_t read_buf[TEST_SIZE];
	int ret;

	/* 1. 테스트 패턴 생성 */
	for (int i = 0; i < TEST_SIZE; i++) {
		write_buf[i] = (uint8_t)(i ^ 0xA5);
	}

	/* 2. Erase */
	printk("  [1] Erase (offset=0x%06X, size=%d)...", TEST_OFFSET, PAGE_SIZE);
	ret = flash_erase(flash_dev, TEST_OFFSET, PAGE_SIZE);
	if (ret < 0) {
		printk(" FAIL (%d)\n", ret);
		return ret;
	}
	printk(" OK\n");

	/* 3. Erase 확인 (0xFF) */
	memset(read_buf, 0, TEST_SIZE);
	ret = flash_read(flash_dev, TEST_OFFSET, read_buf, TEST_SIZE);
	if (ret < 0) {
		printk("  [2] Read after erase... FAIL (%d)\n", ret);
		return ret;
	}
	bool erased = true;
	for (int i = 0; i < TEST_SIZE; i++) {
		if (read_buf[i] != 0xFF) {
			erased = false;
			break;
		}
	}
	printk("  [2] Erase verify... %s\n", erased ? "OK (all 0xFF)" : "FAIL");
	if (!erased) {
		return -1;
	}

	/* 4. Write */
	printk("  [3] Write %d bytes...", TEST_SIZE);
	ret = flash_write(flash_dev, TEST_OFFSET, write_buf, TEST_SIZE);
	if (ret < 0) {
		printk(" FAIL (%d)\n", ret);
		return ret;
	}
	printk(" OK\n");

	/* 5. Read back & verify */
	memset(read_buf, 0, TEST_SIZE);
	ret = flash_read(flash_dev, TEST_OFFSET, read_buf, TEST_SIZE);
	if (ret < 0) {
		printk("  [4] Read back... FAIL (%d)\n", ret);
		return ret;
	}

	if (memcmp(write_buf, read_buf, TEST_SIZE) == 0) {
		printk("  [4] Read verify... OK (data match)\n");
	} else {
		printk("  [4] Read verify... FAIL (data mismatch)\n");
		for (int i = 0; i < 16; i++) {
			printk("    [%02X] wrote=0x%02X read=0x%02X %s\n",
			       i, write_buf[i], read_buf[i],
			       write_buf[i] == read_buf[i] ? "" : "<<");
		}
		return -1;
	}

	/* 6. Erase (원상복구) */
	printk("  [5] Cleanup erase...");
	ret = flash_erase(flash_dev, TEST_OFFSET, PAGE_SIZE);
	if (ret < 0) {
		printk(" FAIL (%d)\n", ret);
		return ret;
	}
	printk(" OK\n");

	return 0;
}

int main(void)
{
	int ret;
	const struct device *gpio0;

	printk("\n========================================\n");
	printk("  HW Test - Flash (MX25R1635F)\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  QSPI Flash: MX25R1635F (16Mbit/2MB)\n");
	printk("  Test offset: 0x%06X\n", TEST_OFFSET);
	printk("  Test size: %d bytes\n", TEST_SIZE);
	printk("========================================\n\n");

	/* LED */
	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (device_is_ready(gpio0)) {
		gpio_pin_configure(gpio0, PIN_LED_EN, GPIO_OUTPUT_LOW);
	}

	/* Flash 디바이스 */
	flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r16));
	if (!device_is_ready(flash_dev)) {
		printk("ERROR: Flash device not ready\n");
		return -1;
	}
	printk("Flash device ready\n\n");

	/* 셀프 테스트 실행 */
	printk("=== Flash Self Test ===\n");
	ret = flash_self_test();
	printk("\n>>> Result: %s <<<\n\n", ret == 0 ? "PASS" : "FAIL");

	/* LED 점멸로 결과 표시 */
	while (1) {
		if (ret == 0) {
			/* PASS: 1초 점멸 */
			gpio_pin_toggle(gpio0, PIN_LED_EN);
			k_sleep(K_MSEC(1000));
		} else {
			/* FAIL: 빠른 점멸 */
			gpio_pin_toggle(gpio0, PIN_LED_EN);
			k_sleep(K_MSEC(200));
		}
	}

	return 0;
}
