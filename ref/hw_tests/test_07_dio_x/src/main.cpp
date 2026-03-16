/*
 * Test 07: 12V DIO_X Input Test
 *
 * Purpose: Test 12V to 3.3V level conversion (PC817 optocoupler)
 * Pin: DIO_X (P0.10)
 *
 * Circuit behavior (PC817):
 *   - 12V input -> LED on -> Phototransistor on -> Output LOW
 *   - No input  -> LED off -> Phototransistor off -> Output HIGH (pull-up)
 *
 * Test procedure:
 * 1. Apply 12V to DIO_X input -> should read LOW
 * 2. Remove 12V -> should read HIGH
 */

#include <Arduino.h>
#include "pin_config.h"
#include "debug_uart.h"

volatile bool dio_changed = false;
volatile bool dio_state = false;

void dio_isr() {
    dio_changed = true;
    dio_state = digitalRead(DIO_X_PIN);
}

void show_status() {
    bool state = digitalRead(DIO_X_PIN);

    debug_separator();
    debug_println("DIO_X (P0.10) Status:");
    debug_print("  Pin state: ");
    debug_println(state ? "HIGH" : "LOW");
    debug_print("  12V input: ");
    debug_println(state ? "NOT applied (open)" : "APPLIED (12V)");
    debug_separator();
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  r - Read current state");
    debug_println("  c - Continuous monitor (press any key to stop)");
    debug_println("  h - Show help");
    debug_println("");
    debug_println("Note: Optocoupler inverts the signal!");
    debug_println("  12V applied -> Output LOW");
    debug_println("  12V removed -> Output HIGH");
    debug_println("");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("DIO_X 12V Input Test");

    debug_println("Pin Configuration:");
    debug_print("  DIO_X Pin: P0.");
    debug_println(DIO_X_PIN);
    debug_println("  Level shifter: PC817 optocoupler");
    debug_println("");

    // Configure DIO_X as input with pull-up
    pinMode(DIO_X_PIN, INPUT_PULLUP);

    // Attach interrupt for edge detection
    attachInterrupt(digitalPinToInterrupt(DIO_X_PIN), dio_isr, CHANGE);

    debug_result("DIO_X initialized", true);

    show_status();
    show_help();
}

bool continuous_mode = false;
bool last_state = false;

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
            continuous_mode = true;
            last_state = digitalRead(DIO_X_PIN);
        } else if (c == 'h' || c == 'H') {
            show_help();
        }
    }

    // Handle interrupt-detected changes
    if (dio_changed) {
        dio_changed = false;
        debug_print("[IRQ] DIO_X changed to: ");
        debug_print(dio_state ? "HIGH" : "LOW");
        debug_print(" (12V ");
        debug_print(dio_state ? "removed" : "applied");
        debug_println(")");
    }

    // Continuous polling mode
    if (continuous_mode) {
        bool current = digitalRead(DIO_X_PIN);
        if (current != last_state) {
            debug_print("DIO_X: ");
            debug_print(current ? "HIGH" : "LOW");
            debug_print(" (12V ");
            debug_print(current ? "removed" : "applied");
            debug_println(")");
            last_state = current;
        }
        delay(50);
    }
}
