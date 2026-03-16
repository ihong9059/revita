/*
 * Test 05: Battery ADC Test
 *
 * Purpose: Read battery voltage via ADC
 * Pin: BAT_AIN (P0.05 / AIN3)
 *
 * Note: nRF52840 ADC is 14-bit (0-16383)
 *       Reference voltage is 3.6V (internal)
 *       Battery voltage divider: RDIV1=1M, RDIV2=1M (1:2 ratio)
 */

#include <Arduino.h>
#include "pin_config.h"
#include "debug_uart.h"

// ADC configuration
#define ADC_RESOLUTION      14
#define ADC_MAX_VALUE       16383
#define ADC_REF_VOLTAGE     3.6f

// Voltage divider ratio (RDIV1 = RDIV2 = 1M, so ratio = 2)
#define VOLTAGE_DIVIDER     2.0f

// Smoothing
#define NUM_SAMPLES         10

float read_battery_voltage() {
    uint32_t sum = 0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += analogRead(BAT_AIN_PIN);
        delay(5);
    }

    float avg = (float)sum / NUM_SAMPLES;
    float voltage = (avg / ADC_MAX_VALUE) * ADC_REF_VOLTAGE * VOLTAGE_DIVIDER;

    return voltage;
}

uint16_t read_adc_raw() {
    return analogRead(BAT_AIN_PIN);
}

void show_battery_info() {
    debug_separator();
    debug_println("Battery ADC Reading:");

    uint16_t raw = read_adc_raw();
    float voltage = read_battery_voltage();

    debug_print("  Raw ADC: ");
    debug_println(raw);

    debug_print("  Voltage: ");
    debug_print(voltage);
    debug_println(" V");

    // Battery status estimation (for 1S LiPo)
    debug_print("  Status:  ");
    if (voltage > 4.1) {
        debug_println("Full (>4.1V)");
    } else if (voltage > 3.7) {
        debug_println("Good (3.7-4.1V)");
    } else if (voltage > 3.5) {
        debug_println("Low (3.5-3.7V)");
    } else if (voltage > 3.0) {
        debug_println("Critical (<3.5V)");
    } else {
        debug_println("No battery or error");
    }

    debug_separator();
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("Battery ADC Test");

    debug_println("ADC Configuration:");
    debug_print("  ADC Pin: P0.");
    debug_println(BAT_AIN_PIN);
    debug_print("  Resolution: ");
    debug_print(ADC_RESOLUTION);
    debug_println(" bits");
    debug_print("  Reference: ");
    debug_print(ADC_REF_VOLTAGE);
    debug_println(" V");
    debug_print("  Divider ratio: ");
    debug_print(VOLTAGE_DIVIDER);
    debug_println(":1");
    debug_println("");

    // Configure ADC
    analogReference(AR_INTERNAL);
    analogReadResolution(ADC_RESOLUTION);

    debug_println("Commands:");
    debug_println("  r - Read once");
    debug_println("  c - Continuous read (press any key to stop)");
    debug_println("");

    show_battery_info();
}

bool continuous_mode = false;

void loop() {
    if (debug_available()) {
        char c = debug_read();

        if (continuous_mode) {
            continuous_mode = false;
            debug_println("");
            debug_println("Continuous mode stopped.");
        } else if (c == 'r' || c == 'R') {
            show_battery_info();
        } else if (c == 'c' || c == 'C') {
            debug_println("Continuous mode (press any key to stop):");
            continuous_mode = true;
        }
    }

    if (continuous_mode) {
        float voltage = read_battery_voltage();
        debug_print("Battery: ");
        debug_print(voltage);
        debug_println(" V");
        delay(500);
    }
}
