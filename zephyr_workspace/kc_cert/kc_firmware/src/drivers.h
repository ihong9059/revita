#ifndef DRIVERS_H
#define DRIVERS_H

#include <stdint.h>
#include <stdbool.h>

int drv_init(void);

/* GPIO Output */
void drv_12v_set(bool on);
void drv_buzzer_set(bool on);
void drv_led_set(uint16_t mode);  /* 0=OFF, 1=ON, 2=blink1, 3=blink2 */
void drv_led_tick(void);          /* call every 100ms for blink */

/* Valve */
void drv_valve_cw(bool is_x);    /* Open */
void drv_valve_ccw(bool is_x);   /* Close */
void drv_valve_stop(bool is_x);

/* GPIO Input */
int drv_btn_read(void);
int drv_vib_read(void);

/* ADC */
int32_t drv_adc_read_mv(void);   /* returns battery mV or -1 */

/* Flash */
int drv_flash_self_test(void);    /* returns 0=PASS, -1=FAIL */

#endif
