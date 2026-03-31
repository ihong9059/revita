#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>

/* BAT_AIN: P0.31 = AIN7, VCC/2 분압 */
#define ADC_CHANNEL     7
#define ADC_RESOLUTION  12
#define ADC_GAIN        ADC_GAIN_1_6    /* 0~3.6V 범위 */
#define ADC_REFERENCE   ADC_REF_INTERNAL /* 0.6V 내부 기준 */

/* LED_EN: P0.15 */
#define PIN_LED_EN      15

static const struct device *adc_dev;
static int16_t adc_buffer;

static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	.channel_id = ADC_CHANNEL,
	.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput7,
};

static struct adc_sequence sequence = {
	.channels = BIT(ADC_CHANNEL),
	.buffer = &adc_buffer,
	.buffer_size = sizeof(adc_buffer),
	.resolution = ADC_RESOLUTION,
};

int main(void)
{
	int ret;
	const struct device *gpio0;
	bool led_on = false;

	printk("\n========================================\n");
	printk("  HW Test - ADC Battery Voltage\n");
	printk("  REVITA_LINK_v1 / RAK4631\n");
	printk("========================================\n");
	printk("  BAT_AIN: P0.31 (AIN7)\n");
	printk("  분압: VCC/2 (1M/1M)\n");
	printk("  ADC: 12bit, Gain 1/6, Ref 0.6V\n");
	printk("  측정 범위: 0 ~ 3.6V (ADC)\n");
	printk("  실제 전압: ADC × 2 (분압 보정)\n");
	printk("========================================\n\n");

	/* LED */
	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (device_is_ready(gpio0)) {
		gpio_pin_configure(gpio0, PIN_LED_EN, GPIO_OUTPUT_LOW);
	}

	/* ADC 초기화 */
	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (!device_is_ready(adc_dev)) {
		printk("ERROR: ADC not ready\n");
		return -1;
	}

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret < 0) {
		printk("ERROR: ADC channel setup failed: %d\n", ret);
		return -1;
	}
	printk("ADC ready (AIN7)\n\n");

	while (1) {
		ret = adc_read(adc_dev, &sequence);
		if (ret < 0) {
			printk("ADC read error: %d\n", ret);
		} else {
			/* ADC 값 → 전압 변환
			 * Gain 1/6, Ref 0.6V → 풀스케일 = 0.6V × 6 = 3.6V
			 * ADC 전압 = (adc_buffer / 4096) × 3600 mV
			 * 실제 배터리 전압 = ADC 전압 × 2 (1M/1M 분압)
			 */
			int32_t adc_mv = (adc_buffer * 3600) / 4096;
			int32_t bat_mv = adc_mv * 2;

			printk("ADC raw: %d, ADC: %d mV, Battery: %d mV (%d.%03d V)\n",
			       adc_buffer, adc_mv, bat_mv,
			       bat_mv / 1000, bat_mv % 1000);
		}

		/* LED 토글 */
		led_on = !led_on;
		gpio_pin_set_raw(gpio0, PIN_LED_EN, led_on ? 1 : 0);

		k_sleep(K_MSEC(1000));
	}

	return 0;
}
