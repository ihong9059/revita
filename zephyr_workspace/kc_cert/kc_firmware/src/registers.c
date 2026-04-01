#include "registers.h"
#include "drivers.h"
#include "lora.h"
#include "sensor.h"
#include "sensor2.h"
#include <zephyr/kernel.h>

/* Device state */
static struct {
	/* 0x0000~0x0004: Device info */
	uint16_t device_id_h;
	uint16_t device_id_l;
	uint16_t fw_version;
	uint16_t hw_version;
	uint16_t test_mode;

	/* 0x0010~0x0016: Power/Sensors */
	uint16_t bat_voltage;
	uint16_t bat_percent;
	uint16_t vib_status;
	uint16_t vib_count;
	uint16_t btn_status;
	uint16_t supply_12v;
	uint16_t input_changed;  /* 0x0016: bit0=btn_changed, bit1=vib_changed */

	/* Internal: previous state for change detection */
	int prev_btn;
	int prev_vib;

	/* 0x0020~0x0023: Valve */
	uint16_t valve_x_state;
	uint16_t valve_y_state;
	uint16_t valve_x_pin;
	uint16_t valve_y_pin;

	/* 0x0040~0x0041: Output */
	uint16_t buzzer_state;
	uint16_t led_state;

	/* 0x0070~0x0071: Flash */
	uint16_t flash_state;
	uint16_t flash_test;

	/* 0x0080~0x0085: Soil Sensor (moisture/temp/EC) */
	uint16_t sensor_mois;
	int16_t  sensor_temp;
	uint16_t sensor_ec;
	uint16_t sensor_status;
	uint16_t sensor_read_cnt;
	uint16_t sensor_err_cnt;

	/* 0x0090~0x0094: Sensor2 (온습도) */
	uint16_t sensor2_humi;
	int16_t  sensor2_temp;
	uint16_t sensor2_status;
	uint16_t sensor2_read_cnt;
	uint16_t sensor2_err_cnt;

	/* 0x0050~0x005C: LoRa (cached from lora module) */
	uint16_t lora_state;
	uint16_t lora_freq;
	int16_t  lora_power;
	int16_t  lora_rssi;
	int16_t  lora_snr;
	uint16_t lora_tx_cnt;
	uint16_t lora_rx_cnt;
	uint16_t lora_rx_err;
	uint16_t lora_count_hi;
	uint16_t lora_count_lo;
	uint16_t lora_last_seq;
	uint16_t lora_uptime_hi;
	uint16_t lora_uptime_lo;

	/* Timers */
	int64_t buzzer_off_time;
	int64_t valve_x_off_time;
	int64_t valve_y_off_time;
} state;

void reg_init(void)
{
	state.device_id_h = 0x5245;  /* "RE" */
	state.device_id_l = 0x564C;  /* "VL" */
	state.fw_version = 0x0100;   /* v1.0 */
	state.hw_version = 0x0001;
	state.test_mode = 0;
	state.input_changed = 0;
	state.prev_btn = -1;  /* 초기: 미확정 */
	state.prev_vib = -1;
}

int32_t reg_read(uint16_t addr)
{
	switch (addr) {
	/* Device info */
	case 0x0000: return state.device_id_h;
	case 0x0001: return state.device_id_l;
	case 0x0002: return state.fw_version;
	case 0x0003: return state.hw_version;
	case 0x0004: return state.test_mode;

	/* Power/Sensors */
	case 0x0010: return state.bat_voltage;
	case 0x0011: return state.bat_percent;
	case 0x0012: return state.vib_status;
	case 0x0013: return state.vib_count;
	case 0x0014: return state.btn_status;
	case 0x0015: return state.supply_12v;
	case 0x0016: {
		/* 읽으면 플래그 클리어 */
		uint16_t val = state.input_changed;
		state.input_changed = 0;
		return val;
	}

	/* Valve */
	case 0x0020: return state.valve_x_state;
	case 0x0021: return state.valve_y_state;
	case 0x0022: return state.valve_x_pin;
	case 0x0023: return state.valve_y_pin;

	/* Output */
	case 0x0040: return state.buzzer_state;
	case 0x0041: return state.led_state;

	/* Flash */
	case 0x0070: return state.flash_state;
	case 0x0071: return state.flash_test;

	/* Soil Sensor */
	case 0x0080: return state.sensor_mois;
	case 0x0081: return (uint16_t)state.sensor_temp;
	case 0x0082: return state.sensor_ec;
	case 0x0083: return state.sensor_status;
	case 0x0084: return state.sensor_read_cnt;
	case 0x0085: return state.sensor_err_cnt;

	/* Sensor2 (온습도) */
	case 0x0090: return state.sensor2_humi;
	case 0x0091: return (uint16_t)state.sensor2_temp;
	case 0x0092: return state.sensor2_status;
	case 0x0093: return state.sensor2_read_cnt;
	case 0x0094: return state.sensor2_err_cnt;

	/* LoRa status */
	case 0x0050: return state.lora_state;
	case 0x0051: return state.lora_freq;
	case 0x0052: return (uint16_t)state.lora_power;
	case 0x0053: return (uint16_t)state.lora_rssi;
	case 0x0054: return (uint16_t)state.lora_snr;
	case 0x0055: return state.lora_tx_cnt;
	case 0x0056: return state.lora_rx_cnt;
	case 0x0057: return state.lora_rx_err;
	case 0x0058: return state.lora_count_lo;
	case 0x0059: return state.lora_count_hi;
	case 0x005A: return state.lora_last_seq;
	case 0x005B: return state.lora_uptime_lo;
	case 0x005C: return state.lora_uptime_hi;

	default:
		return -1;  /* Illegal address */
	}
}

static void valve_control(const char *axis, uint16_t cmd, uint16_t duration)
{
	bool is_x = (axis[0] == 'x');
	uint16_t *valve_state = is_x ? &state.valve_x_state : &state.valve_y_state;
	uint16_t *valve_pin = is_x ? &state.valve_x_pin : &state.valve_y_pin;
	int64_t *off_time = is_x ? &state.valve_x_off_time : &state.valve_y_off_time;

	switch (cmd) {
	case 0: /* Close */
		drv_valve_ccw(is_x);
		*valve_state = 2; /* 동작중 */
		*valve_pin = 0x02; /* EN_B */
		break;
	case 1: /* Open */
		drv_valve_cw(is_x);
		*valve_state = 2;
		*valve_pin = 0x01; /* EN_A */
		break;
	case 2: /* Stop */
		drv_valve_stop(is_x);
		*valve_state = 0;
		*valve_pin = 0;
		*off_time = 0;
		return;
	default:
		return;
	}

	if (duration > 0) {
		*off_time = k_uptime_get() + (int64_t)duration * 1000;
	} else {
		*off_time = 0;
	}

	/* 12V auto enable */
	drv_12v_set(true);
	state.supply_12v = 1;
}

static void all_stop(void)
{
	drv_valve_stop(true);
	drv_valve_stop(false);
	drv_buzzer_set(false);
	drv_led_set(0);
	drv_12v_set(false);

	state.valve_x_state = 0;
	state.valve_y_state = 0;
	state.valve_x_pin = 0;
	state.valve_y_pin = 0;
	state.buzzer_state = 0;
	state.led_state = 0;
	state.supply_12v = 0;
	state.buzzer_off_time = 0;
	state.valve_x_off_time = 0;
	state.valve_y_off_time = 0;

	printk("  ALL STOP\n");
}

int reg_write(uint16_t addr, uint16_t value)
{
	switch (addr) {
	/* Valve X */
	case 0x0100:
		valve_control("x", value, 0);
		return 0;
	case 0x0101: /* X duration - handled via 0x10 with 0x0100 */
		return 0;
	case 0x0102:
		valve_control("y", value, 0);
		return 0;
	case 0x0103: /* Y duration */
		return 0;

	/* Buzzer */
	case 0x0120:
		if (value == 0) {
			drv_buzzer_set(false);
			state.buzzer_state = 0;
			state.buzzer_off_time = 0;
		} else if (value == 0xFFFF) {
			drv_12v_set(true);
			state.supply_12v = 1;
			drv_buzzer_set(true);
			state.buzzer_state = 1;
			state.buzzer_off_time = 0;
		} else {
			drv_12v_set(true);
			state.supply_12v = 1;
			drv_buzzer_set(true);
			state.buzzer_state = 1;
			state.buzzer_off_time = k_uptime_get() + (int64_t)value * 1000;
		}
		printk("  Buzzer: %s (duration=%u)\n",
		       value ? "ON" : "OFF", value);
		return 0;

	/* LED */
	case 0x0121:
		drv_led_set(value);
		state.led_state = value;
		printk("  LED: mode=%u\n", value);
		return 0;

	/* 12V */
	case 0x0122:
		drv_12v_set(value != 0);
		state.supply_12v = value ? 1 : 0;
		printk("  12V: %s\n", value ? "ON" : "OFF");
		return 0;

	/* Flash test */
	case 0x0150:
		if (value == 2) {
			printk("  Flash self-test...\n");
			int ret = drv_flash_self_test();
			state.flash_test = (ret == 0) ? 1 : 2;
			printk("  Flash result: %s\n",
			       state.flash_test == 1 ? "PASS" : "FAIL");
		}
		return 0;

	/* LoRa control */
	case 0x0130:
		if (value == 0) {
			lora_rx_stop();
			printk("  LoRa: STOP\n");
		} else if (value == 4) {
			lora_rx_start();
			printk("  LoRa: RX START\n");
		}
		return 0;

	/* ALL STOP */
	case 0x01F0:
		if (value == 1) {
			all_stop();
		}
		return 0;

	/* Test mode */
	case 0x01FF:
		state.test_mode = value ? 1 : 0;
		if (state.test_mode) {
			printk("  === TEST MODE ENTER ===\n");
		} else {
			all_stop();
			printk("  === TEST MODE EXIT ===\n");
		}
		return 0;

	default:
		return -1;  /* Illegal address */
	}
}

void reg_update_sensors(void)
{
	int64_t now = k_uptime_get();

	/* ADC */
	int32_t mv = drv_adc_read_mv();
	if (mv >= 0) {
		state.bat_voltage = mv;
		/* LiFePO4 1S: 2.5V~3.6V */
		if (mv >= 3600) {
			state.bat_percent = 100;
		} else if (mv <= 2500) {
			state.bat_percent = 0;
		} else {
			state.bat_percent = (mv - 2500) * 100 / 1100;
		}
	}

	/* Button — 변화 감지 */
	int btn_cur = drv_btn_read();
	if (state.prev_btn >= 0 && btn_cur != state.prev_btn) {
		state.input_changed |= 0x01;  /* bit0: btn changed */
		printk("  BTN changed: %d -> %d\n", state.prev_btn, btn_cur);
	}
	state.prev_btn = btn_cur;
	state.btn_status = btn_cur;

	/* Vibration — 변화 감지 */
	int vib_cur = drv_vib_read();
	if (state.prev_vib >= 0 && vib_cur != state.prev_vib) {
		state.input_changed |= 0x02;  /* bit1: vib changed */
		if (vib_cur) {
			state.vib_count++;
		}
		printk("  VIB changed: %d -> %d (count=%d)\n",
		       state.prev_vib, vib_cur, state.vib_count);
	}
	state.prev_vib = vib_cur;
	state.vib_status = vib_cur;

	/* Buzzer auto-off */
	if (state.buzzer_off_time > 0 && now >= state.buzzer_off_time) {
		drv_buzzer_set(false);
		state.buzzer_state = 0;
		state.buzzer_off_time = 0;
		printk("  Buzzer auto-off\n");
	}

	/* Valve X auto-stop */
	if (state.valve_x_off_time > 0 && now >= state.valve_x_off_time) {
		drv_valve_stop(true);
		state.valve_x_state = 0;
		state.valve_x_pin = 0;
		state.valve_x_off_time = 0;
		printk("  Valve X auto-stop\n");
	}

	/* Valve Y auto-stop */
	if (state.valve_y_off_time > 0 && now >= state.valve_y_off_time) {
		drv_valve_stop(false);
		state.valve_y_state = 0;
		state.valve_y_pin = 0;
		state.valve_y_off_time = 0;
		printk("  Valve Y auto-stop\n");
	}
}

void reg_update_sensor(void)
{
	const struct sensor_data *sd = sensor_get_data();
	state.sensor_mois = sd->mois_raw;
	state.sensor_temp = sd->temp_raw;
	state.sensor_ec = sd->ec_raw;
	state.sensor_status = sd->status;
	state.sensor_read_cnt = sd->read_count;
	state.sensor_err_cnt = sd->err_count;
}

void reg_update_sensor2(void)
{
	const struct sensor2_data *sd = sensor2_get_data();
	state.sensor2_humi = sd->humi_raw;
	state.sensor2_temp = sd->temp_raw;
	state.sensor2_status = sd->status;
	state.sensor2_read_cnt = sd->read_count;
	state.sensor2_err_cnt = sd->err_count;
}

void reg_update_lora(void)
{
	const struct lora_status *ls = lora_get_status();
	state.lora_state = ls->state;
	state.lora_freq = ls->freq;
	state.lora_power = ls->power;
	state.lora_rssi = ls->rssi;
	state.lora_snr = ls->snr;
	state.lora_tx_cnt = ls->tx_cnt;
	state.lora_rx_cnt = ls->rx_cnt;
	state.lora_rx_err = ls->rx_err_cnt;
	state.lora_count_hi = (uint16_t)(ls->last_count >> 16);
	state.lora_count_lo = (uint16_t)(ls->last_count & 0xFFFF);
	state.lora_last_seq = ls->last_seq;
	state.lora_uptime_hi = (uint16_t)(ls->last_uptime >> 16);
	state.lora_uptime_lo = (uint16_t)(ls->last_uptime & 0xFFFF);
}
