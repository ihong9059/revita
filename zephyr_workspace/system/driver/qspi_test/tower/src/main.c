/*
 * QSPI Flash basic test - Tower / Node 공용 main
 *
 * 동작:
 *   1. RAK4631 모듈 내장 MX25R1635F (16Mbit / 2MB) 디바이스 readiness 확인
 *   2. 마지막 섹터(0x1FF000) 4KB 영역에 self-test
 *      [1] erase
 *      [2] read & verify (0xFF)
 *      [3] write 256 bytes (테스트 패턴: i ^ 0xA5)
 *      [4] read back & memcmp 검증
 *      [5] cleanup erase (원상복구)
 *   3. 결과 PASS/FAIL 을 console 에 출력
 *   4. 1초마다 결과 재출력 (사용자가 cutecom 늦게 열어도 보이도록)
 *
 * 출처: kc_cert/hwTest_flash 패턴 (LED 점멸 제거, console 만 사용)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <string.h>

#define TEST_OFFSET   0x1FF000   /* 2MB - 4KB (마지막 섹터) */
#define TEST_SIZE     256
#define PAGE_SIZE     4096

static const struct device *flash_dev;

static int flash_self_test(void)
{
	uint8_t write_buf[TEST_SIZE];
	uint8_t read_buf[TEST_SIZE];
	int ret;

	for (int i = 0; i < TEST_SIZE; i++) {
		write_buf[i] = (uint8_t)(i ^ 0xA5);
	}

	/* [1] Erase */
	printk("  [1] Erase (offset=0x%06X, size=%d)...", TEST_OFFSET, PAGE_SIZE);
	ret = flash_erase(flash_dev, TEST_OFFSET, PAGE_SIZE);
	if (ret < 0) {
		printk(" FAIL (%d)\n", ret);
		return ret;
	}
	printk(" OK\n");

	/* [2] Read after erase → all 0xFF */
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

	/* [3] Write */
	printk("  [3] Write %d bytes...", TEST_SIZE);
	ret = flash_write(flash_dev, TEST_OFFSET, write_buf, TEST_SIZE);
	if (ret < 0) {
		printk(" FAIL (%d)\n", ret);
		return ret;
	}
	printk(" OK\n");

	/* [4] Read back & verify */
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
			printk("      [%02X] wrote=0x%02X read=0x%02X %s\n",
			       i, write_buf[i], read_buf[i],
			       write_buf[i] == read_buf[i] ? "" : "<<");
		}
		return -1;
	}

	/* [5] Cleanup erase */
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

	printk("\n========================================\n");
	printk("  QSPI Flash test (MX25R1635F)\n");
	printk("  RAK4631 internal QSPI NOR\n");
	printk("  Test offset: 0x%06X, size: %d bytes\n",
	       TEST_OFFSET, TEST_SIZE);
	printk("========================================\n");

	flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r16));
	if (!device_is_ready(flash_dev)) {
		printk("[QSPI] Flash device NOT ready!\n");
		while (1) {
			printk("[QSPI] device not ready (forever)\n");
			k_sleep(K_SECONDS(2));
		}
	}
	printk("[QSPI] Flash device ready\n\n");

	printk("=== Flash Self Test ===\n");
	ret = flash_self_test();
	printk("\n>>> Result: %s <<<\n\n", ret == 0 ? "PASS" : "FAIL");

	/* 결과 주기적 재출력 (cutecom 늦게 열어도 보이도록) */
	uint32_t tick = 0;
	while (1) {
		tick++;
		printk("[QSPI] tick=%u  result=%s\n", tick,
		       ret == 0 ? "PASS" : "FAIL");
		k_sleep(K_SECONDS(2));
	}

	return 0;
}
