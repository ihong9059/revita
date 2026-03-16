/*
 * Test 10: Vibration Sensor Test
 *
 * Purpose: Test vibration sensor (SW-18010P)
 * Pin: VIB_SENSE (P0.21)
 *
 * Sensor behavior:
 *   - Vibration detected -> momentary contact closure
 *   - Outputs pulses during vibration
 */

#include <Arduino.h>
#include "pin_config.h"
#include "debug_uart.h"

volatile uint32_t pulse_count = 0;
volatile unsigned long last_pulse_time = 0;

void vib_isr() {
    pulse_count++;
    last_pulse_time = millis();
}

void show_status() {
    debug_separator();
    debug_println("Vibration Sensor Status:");
    debug_print("  Pin state: ");
    debug_println(digitalRead(VIB_SENSE_PIN) ? "HIGH" : "LOW");
    debug_print("  Total pulses: ");
    debug_println((int)pulse_count);
    debug_separator();
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  r - Read current state");
    debug_println("  c - Continuous monitor (press any key to stop)");
    debug_println("  z - Reset pulse count");
    debug_println("  h - Show help");
    debug_println("");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("Vibration Sensor Test");

    debug_println("Pin Configuration:");
    debug_print("  VIB_SENSE Pin: P0.");
    debug_println(VIB_SENSE_PIN);
    debug_println("  Sensor: SW-18010P");
    debug_println("");

    // Configure VIB_SENSE as input with pull-up
    pinMode(VIB_SENSE_PIN, INPUT_PULLUP);

    // Attach interrupt for pulse counting
    attachInterrupt(digitalPinToInterrupt(VIB_SENSE_PIN), vib_isr, FALLING);

    debug_result("Vibration sensor initialized", true);

    show_status();
    show_help();
}

bool continuous_mode = false;
uint32_t last_count = 0;
unsigned long last_report = 0;

void loop() {
    if (debug_available()) {
        char c = debug_read();

        if (continuous_mode) {
            continuous_mode = false;
            debug_println("");
            debug_println("Continuous mode stopped.");
        } else if (c == 'r' || c == 'R') {
            show_status();
        } else if (c == 'c' || c == 'C') {
            debug_println("Continuous monitor (press any key to stop):");
            debug_println("Shake the board to detect vibration...");
            continuous_mode = true;
            last_count = pulse_count;
            last_report = millis();
        } else if (c == 'z' || c == 'Z') {
            pulse_count = 0;
            debug_println("Pulse count reset to 0");
        } else if (c == 'h' || c == 'H') {
            show_help();
        }
    }

    // Continuous monitoring mode
    if (continuous_mode) {
        unsigned long now = millis();

        // Report every 500ms
        if (now - last_report >= 500) {
            uint32_t current = pulse_count;
            uint32_t delta = current - last_count;

            if (delta > 0) {
                debug_print("Vibration! Pulses in 500ms: ");
                debug_print((int)delta);
                debug_print(" (Total: ");
                debug_print((int)current);
                debug_println(")");
            }

            last_count = current;
            last_report = now;
        }

        // Also show immediate detection
        static unsigned long last_detect = 0;
        if (last_pulse_time > last_detect) {
            debug_println("* Vibration detected!");
            last_detect = last_pulse_time;
        }
    }
}
