#include "drivers.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/flash.h>
#include <string.h>

/* Pin definitions */
#define PIN_12V_EN      17  /* P0.17 */
#define PIN_BUZZER_EN   24  /* P0.24 */
#define PIN_LED_EN      15  /* P0.15 */
#define PIN_BTN          5  /* P0.05 */
#define PIN_VIB_SENSE   21  /* P0.21 */

/* X axis motor (gpio0) */
#define PIN_X_EN_A      14  /* P0.14 */
#define PIN_X_EN_B      13  /* P0.13 */

/* Y axis motor */
#define PIN_Y_EN_A      25  /* P0.25 - gpio0 */
#define PIN_Y_EN_B       1  /* P1.01 - gpio1 */

/* ADC */
#define ADC_CHANNEL      7  /* AIN7 = P0.31 */

static const struct device *gpio0;
static const struct device *gpio1;
static const struct device *adc_dev;
static const struct device *flash_dev;

static uint16_t led_mode;
static uint8_t led_blink_counter;

/* ADC config */
static int16_t adc_buffer;
static struct adc_channel_cfg adc_cfg = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	.channel_id = ADC_CHANNEL,
	.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput7,
};
static struct adc_sequence adc_seq = {
	.channels = BIT(ADC_CHANNEL),
	.buffer = &adc_buffer,
	.buffer_size = sizeof(adc_buffer),
	.resolution = 12,
};

int drv_init(void)
{
	/* GPIO0 */
	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio0)) {
		printk("ERROR: gpio0 not ready\n");
		return -1;
	}

	/* GPIO1 */
	gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
	if (!device_is_ready(gpio1)) {
		printk("ERROR: gpio1 not ready\n");
		return -1;
	}

	/* Output pins */
	gpio_pin_configure(gpio0, PIN_12V_EN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_BUZZER_EN, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_LED_EN, GPIO_OUTPUT_LOW);

	/* Motor X */
	gpio_pin_configure(gpio0, PIN_X_EN_A, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio0, PIN_X_EN_B, GPIO_OUTPUT_LOW);

	/* Motor Y */
	gpio_pin_configure(gpio0, PIN_Y_EN_A, GPIO_OUTPUT_LOW);
	gpio_pin_configure(gpio1, PIN_Y_EN_B, GPIO_OUTPUT_LOW);

	/* Input pins */
	gpio_pin_configure(gpio0, PIN_BTN, GPIO_INPUT | GPIO_PULL_DOWN);
	gpio_pin_configure(gpio0, PIN_VIB_SENSE, GPIO_INPUT | GPIO_PULL_UP);

	/* ADC */
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (!device_is_ready(adc_dev)) {
		printk("WARN: ADC not ready\n");
		adc_dev = NULL;
	} else {
		adc_channel_setup(adc_dev, &adc_cfg);
	}

	/* Flash */
	flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r16));
	if (!device_is_ready(flash_dev)) {
		printk("WARN: Flash not ready\n");
		flash_dev = NULL;
	}

	printk("Drivers initialized\n");
	return 0;
}

void drv_12v_set(bool on)
{
	gpio_pin_set_raw(gpio0, PIN_12V_EN, on ? 1 : 0);
}

void drv_buzzer_set(bool on)
{
	gpio_pin_set_raw(gpio0, PIN_BUZZER_EN, on ? 1 : 0);
}

void drv_led_set(uint16_t mode)
{
	led_mode = mode;
	led_blink_counter = 0;

	switch (mode) {
	case 0:
		gpio_pin_set_raw(gpio0, PIN_LED_EN, 0);
		break;
	case 1:
		gpio_pin_set_raw(gpio0, PIN_LED_EN, 1);
		break;
	default:
		break;
	}
}

void drv_led_tick(void)
{
	if (led_mode < 2) {
		return;
	}

	led_blink_counter++;

	if (led_mode == 2) {
		/* 1박자 점멸: 500ms ON / 500ms OFF */
		if (led_blink_counter >= 5) {
			gpio_pin_toggle(gpio0, PIN_LED_EN);
			led_blink_counter = 0;
		}
	} else if (led_mode == 3) {
		/* 2박자 점멸: 200ms ON / 200ms OFF / 200ms ON / 600ms OFF */
		uint8_t pos = led_blink_counter % 12;
		if (pos == 0) {
			gpio_pin_set_raw(gpio0, PIN_LED_EN, 1);
		} else if (pos == 2) {
			gpio_pin_set_raw(gpio0, PIN_LED_EN, 0);
		} else if (pos == 4) {
			gpio_pin_set_raw(gpio0, PIN_LED_EN, 1);
		} else if (pos == 6) {
			gpio_pin_set_raw(gpio0, PIN_LED_EN, 0);
		}
	}
}

void drv_valve_cw(bool is_x)
{
	if (is_x) {
		gpio_pin_set_raw(gpio0, PIN_X_EN_B, 0);
		gpio_pin_set_raw(gpio0, PIN_X_EN_A, 1);
	} else {
		gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
		gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 1);
	}
}

void drv_valve_ccw(bool is_x)
{
	if (is_x) {
		gpio_pin_set_raw(gpio0, PIN_X_EN_A, 0);
		gpio_pin_set_raw(gpio0, PIN_X_EN_B, 1);
	} else {
		gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
		gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 1);
	}
}

void drv_valve_stop(bool is_x)
{
	if (is_x) {
		gpio_pin_set_raw(gpio0, PIN_X_EN_A, 0);
		gpio_pin_set_raw(gpio0, PIN_X_EN_B, 0);
	} else {
		gpio_pin_set_raw(gpio0, PIN_Y_EN_A, 0);
		gpio_pin_set_raw(gpio1, PIN_Y_EN_B, 0);
	}
}

int drv_btn_read(void)
{
	return gpio_pin_get_raw(gpio0, PIN_BTN);
}

int drv_vib_read(void)
{
	/* Active low: 진동 시 LOW */
	return gpio_pin_get_raw(gpio0, PIN_VIB_SENSE) ? 0 : 1;
}

int32_t drv_adc_read_mv(void)
{
	if (adc_dev == NULL) {
		return -1;
	}

	int ret = adc_read(adc_dev, &adc_seq);
	if (ret < 0) {
		return -1;
	}

	/* Gain 1/6, Ref 0.6V → full scale 3.6V, × 2 for voltage divider */
	int32_t adc_mv = (adc_buffer * 3600) / 4096;
	return adc_mv * 2;
}

int drv_flash_self_test(void)
{
	if (flash_dev == NULL) {
		return -1;
	}

	uint8_t write_buf[256];
	uint8_t read_buf[256];
	uint32_t offset = 0x1FF000; /* Last 4KB sector */

	for (int i = 0; i < 256; i++) {
		write_buf[i] = (uint8_t)(i ^ 0xA5);
	}

	if (flash_erase(flash_dev, offset, 4096) < 0) {
		return -1;
	}
	if (flash_write(flash_dev, offset, write_buf, 256) < 0) {
		return -1;
	}
	if (flash_read(flash_dev, offset, read_buf, 256) < 0) {
		return -1;
	}
	if (memcmp(write_buf, read_buf, 256) != 0) {
		return -1;
	}

	flash_erase(flash_dev, offset, 4096);
	return 0;
}
