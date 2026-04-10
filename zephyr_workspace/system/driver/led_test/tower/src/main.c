/*
 * LED blink test - Tower
 *
 * LED: MCP23017 GPB6 (LED_EN)
 * I2C0: SDA=P0.13, SCL=P0.14, 100kHz
 * 1초 간격으로 ON/OFF 토글, 디버그 콘솔에 상태 출력
 *
 * 참고: tower.h
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* MCP23017 registers */
#define MCP_REG_IODIRB   0x01
#define MCP_REG_OLATB    0x15

/* GPB6 = LED_EN */
#define GPB6_LED_EN      (1U << 6)

static const struct device *i2c_dev;
static uint8_t mcp_addr;
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

static void i2c_scan(void)
{
	printk("[LED] I2C scan (0x08-0x77)...\n");
	int found = 0;
	for (uint8_t a = 0x08; a <= 0x77; a++) {
		uint8_t dummy;
		if (i2c_read(i2c_dev, &dummy, 1, a) == 0) {
			printk("[LED]   0x%02X found\n", a);
			found++;
		}
	}
	printk("[LED] scan done, %d device(s)\n", found);
}

static int mcp_init(void)
{
	if (!device_is_ready(i2c_dev)) {
		printk("[LED] i2c0 NOT ready\n");
		return -1;
	}
	printk("[LED] i2c0 ready\n");

	k_msleep(200);

	i2c_scan();

	/* MCP23017 probe (0x20-0x27) */
	for (uint8_t a = 0x20; a <= 0x27; a++) {
		uint8_t reg = 0x00, val;
		if (i2c_write_read(i2c_dev, a, &reg, 1, &val, 1) == 0) {
			mcp_addr = a;
			printk("[LED] MCP23017 found at 0x%02X\n", a);
			break;
		}
	}

	if (mcp_addr == 0) {
		printk("[LED] MCP23017 NOT found at 0x20-0x27\n");
		return -2;
	}

	/* GPB6 (LED_EN) → output */
	uint8_t iodirb;
	if (mcp_read(MCP_REG_IODIRB, &iodirb) < 0) return -3;
	iodirb &= ~GPB6_LED_EN;
	if (mcp_write(MCP_REG_IODIRB, iodirb) < 0) return -4;

	/* 캐시 초기화 */
	if (mcp_read(MCP_REG_OLATB, &olatb_cache) < 0) return -5;

	/* LED OFF 으로 시작 */
	olatb_cache &= ~GPB6_LED_EN;
	if (mcp_write(MCP_REG_OLATB, olatb_cache) < 0) return -6;

	return 0;
}

static int set_led(bool on)
{
	if (on) olatb_cache |= GPB6_LED_EN;
	else    olatb_cache &= ~GPB6_LED_EN;
	return mcp_write(MCP_REG_OLATB, olatb_cache);
}

int main(void)
{
	bool led_on = false;
	uint32_t count = 0;

	printk("\n========================================\n");
	printk("  LED blink test - Tower\n");
	printk("  MCP23017 GPB6 = LED_EN\n");
	printk("  pattern: 1s ON / 1s OFF\n");
	printk("========================================\n");

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	int rc = mcp_init();
	if (rc < 0) {
		printk("[LED] MCP23017 init FAIL (rc=%d)\n", rc);
		while (1) {
			k_sleep(K_SECONDS(2));
			printk("[LED] MCP init failed — cannot proceed\n");
		}
	}
	printk("[LED] MCP23017 @ 0x%02X ready\n", mcp_addr);

	while (1) {
		led_on = !led_on;
		set_led(led_on);
		count++;
		printk("[LED] #%u %s\n", count, led_on ? "ON" : "OFF");
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
