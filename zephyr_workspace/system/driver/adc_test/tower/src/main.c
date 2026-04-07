/*
 * ADC battery voltage test - Tower
 *
 * Tower BAT_AIN: P0.05 = AIN3
 * 분압: 1M / 220k → 보정 배수 = (1000 + 220) / 220 = 5.545
 *
 * SAADC: 12-bit, Gain 1/6, Internal 0.6V ref
 *   ADC 풀스케일 = 0.6V × 6 = 3.6V
 *   ADC mV = (raw / 4096) × 3600
 *   VBAT mV = ADC mV × 5.545
 *
 * 출처 패턴: kc_cert/hwTest_adc (Link 1M/1M 분압) → Tower 비율로 보정
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>

#define ADC_CHANNEL     3        /* AIN3 */
#define ADC_RESOLUTION  12

/* 분압 보정 배수 ×1000 (정수 연산용) */
#define DIVIDER_NUM     5545     /* (1000 + 220) / 220 = 5.545 → ×1000 = 5545 */
#define DIVIDER_DEN     1000

static const struct device *adc_dev;
static int16_t adc_buffer;

static struct adc_channel_cfg channel_cfg = {
	.gain             = ADC_GAIN_1_6,
	.reference        = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
	.channel_id       = ADC_CHANNEL,
	.input_positive   = SAADC_CH_PSELP_PSELP_AnalogInput3,
};

static struct adc_sequence sequence = {
	.channels    = BIT(ADC_CHANNEL),
	.buffer      = &adc_buffer,
	.buffer_size = sizeof(adc_buffer),
	.resolution  = ADC_RESOLUTION,
};

int main(void)
{
	int ret;

	printk("\n========================================\n");
	printk("  ADC Battery test - Tower\n");
	printk("  BAT_AIN: P0.05 (AIN3)\n");
	printk("  Divider: 1M / 220k  → x%d.%03d\n",
	       DIVIDER_NUM / DIVIDER_DEN, DIVIDER_NUM % DIVIDER_DEN);
	printk("  ADC: 12bit, Gain 1/6, Ref 0.6V (full=3.6V)\n");
	printk("========================================\n\n");

	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (!device_is_ready(adc_dev)) {
		printk("[ADC] device NOT ready\n");
		return -1;
	}

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret < 0) {
		printk("[ADC] channel setup failed: %d\n", ret);
		return -1;
	}
	printk("[ADC] AIN3 ready\n\n");

	while (1) {
		ret = adc_read(adc_dev, &sequence);
		if (ret < 0) {
			printk("[ADC] read error: %d\n", ret);
		} else {
			int32_t raw = adc_buffer;
			if (raw < 0) raw = 0;

			int32_t adc_mv = (raw * 3600) / 4096;
			int32_t bat_mv = (adc_mv * DIVIDER_NUM) / DIVIDER_DEN;

			printk("[ADC] raw=%4d  pin=%4d mV  VBAT=%5d mV (%d.%03d V)\n",
			       (int)raw, (int)adc_mv, (int)bat_mv,
			       (int)(bat_mv / 1000), (int)(bat_mv % 1000));
		}
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
