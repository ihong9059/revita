/*
 * Buzzer basic test - Tower
 *
 * 동작:
 *   1. MCP23017 (@0x20) probe + IODIR 설정
 *      - GPA2 (12_14V_EN) → output
 *      - GPB1 (BUZZER_EN) → output
 *   2. 12V_EN HIGH (12V 부스트 ON)
 *   3. 2초 간격으로 BUZZER_EN 100ms ON (5회 → 총 10초)
 *   4. 5회 후 BUZZER_EN / 12V_EN 모두 OFF
 *   5. idle (1초마다 상태 출력)
 *
 * 참고: tower.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* MCP23017 registers */
#define MCP_REG_IODIRA   0x00
#define MCP_REG_IODIRB   0x01
#define MCP_REG_OLATA    0x14
#define MCP_REG_OLATB    0x15

/* GPA bits */
#define GPA2_12_14V_EN   (1U << 2)

/* GPB bits */
#define GPB1_BUZZER_EN   (1U << 1)

#define BEEP_INTERVAL_MS  2000
#define BEEP_DURATION_MS  100
#define BEEP_COUNT        5

static const struct device *i2c_dev;
static uint8_t mcp_addr;
static uint8_t olata_cache;
static uint8_t olatb_cache;

static int mcp_write(uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = { reg, val };
	return i2c_write(i2c_dev, buf, 2, mcp_addr);
}

static int mcp_read(uint8_t reg, uint8_t *val)
{
	return i2c_write_read(i2c_dev, mcp_addr, &reg, 1, val, 1);
}

static void i2c_scan(const struct device *bus, const char *name)
{
	printk("[BUZZ] scanning %s ...\n", name);
	int found = 0;
	for (uint8_t a = 0x08; a <= 0x77; a++) {
		uint8_t dummy;
		if (i2c_read(bus, &dummy, 1, a) == 0) {
			printk("[BUZZ]   addr 0x%02X responds\n", a);
			found++;
		}
	}
	printk("[BUZZ] %s scan done — %d device(s) found\n", name, found);
}

static int mcp_init(void)
{
	if (!device_is_ready(i2c_dev)) {
		return -1;
	}

	/* MCP23017 power-on/reset 안정화 */
	k_msleep(100);

	/* 진단: 전체 I2C 주소 스캔 */
	i2c_scan(i2c_dev, "i2c0");

	/* i2c0 에서 0x20-0x27 범위 probe */
	for (uint8_t a = 0x20; a <= 0x27; a++) {
		uint8_t reg = MCP_REG_IODIRA, val;
		if (i2c_write_read(i2c_dev, a, &reg, 1, &val, 1) == 0) {
			mcp_addr = a;
			break;
		}
	}

	if (mcp_addr == 0) {
		return -2;
	}

	/* GPA2 (12V_EN) → output */
	uint8_t iodira;
	if (mcp_read(MCP_REG_IODIRA, &iodira) < 0) return -3;
	iodira &= ~GPA2_12_14V_EN;
	if (mcp_write(MCP_REG_IODIRA, iodira) < 0) return -4;

	/* GPB1 (BUZZER_EN) → output */
	uint8_t iodirb;
	if (mcp_read(MCP_REG_IODIRB, &iodirb) < 0) return -5;
	iodirb &= ~GPB1_BUZZER_EN;
	if (mcp_write(MCP_REG_IODIRB, iodirb) < 0) return -6;

	/* 캐시 초기화 (현재 OLAT 읽기) */
	if (mcp_read(MCP_REG_OLATA, &olata_cache) < 0) return -7;
	if (mcp_read(MCP_REG_OLATB, &olatb_cache) < 0) return -8;

	/* 부저는 OFF 로 시작 */
	olatb_cache &= ~GPB1_BUZZER_EN;
	if (mcp_write(MCP_REG_OLATB, olatb_cache) < 0) return -9;

	return 0;
}

static int set_12v(bool on)
{
	if (on) olata_cache |= GPA2_12_14V_EN;
	else    olata_cache &= ~GPA2_12_14V_EN;
	return mcp_write(MCP_REG_OLATA, olata_cache);
}

static int set_buzzer(bool on)
{
	if (on) olatb_cache |= GPB1_BUZZER_EN;
	else    olatb_cache &= ~GPB1_BUZZER_EN;
	return mcp_write(MCP_REG_OLATB, olatb_cache);
}

int main(void)
{
	printk("\n========================================\n");
	printk("  Buzzer test - Tower\n");
	printk("  MCP23017 @ 0x20\n");
	printk("    GPA2 = 12_14V_EN\n");
	printk("    GPB1 = BUZZER_EN\n");
	printk("  pattern: %d ms beep / %d ms interval / %d times\n",
	       BEEP_DURATION_MS, BEEP_INTERVAL_MS, BEEP_COUNT);
	printk("========================================\n");

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	int rc = mcp_init();
	if (rc < 0) {
		printk("[BUZZ] MCP23017 init FAIL (rc=%d)\n", rc);
		while (1) {
			k_sleep(K_SECONDS(2));
			printk("[BUZZ] MCP init failed — cannot proceed\n");
		}
	}
	printk("[BUZZ] MCP23017 @ 0x%02X ready\n", mcp_addr);

	/* 12V ON */
	if (set_12v(true) < 0) {
		printk("[BUZZ] 12V_EN write FAIL\n");
		return -1;
	}
	printk("[BUZZ] 12V_EN = HIGH (boost ON)\n");
	k_msleep(50);

	/* Beep 루프 */
	for (int i = 1; i <= BEEP_COUNT; i++) {
		printk("[BUZZ] beep #%d ON\n", i);
		set_buzzer(true);
		k_msleep(BEEP_DURATION_MS);

		set_buzzer(false);
		printk("[BUZZ] beep #%d OFF\n", i);

		if (i < BEEP_COUNT) {
			k_msleep(BEEP_INTERVAL_MS - BEEP_DURATION_MS);
		}
	}

	/* 종료 */
	set_buzzer(false);
	set_12v(false);
	printk("\n[BUZZ] DONE — BUZZER_EN=LOW, 12V_EN=LOW\n");

	uint32_t tick = 0;
	while (1) {
		k_sleep(K_SECONDS(1));
		tick++;
		printk("[BUZZ] idle tick=%u\n", tick);
	}

	return 0;
}
