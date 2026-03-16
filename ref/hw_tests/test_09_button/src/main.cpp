/*
 * Test 09: External Button Test
 *
 * Purpose: Test external button input via MCP23017
 * Pin: BTN (MCP23017 GPA0)
 * Interrupt: INTA (P1.01)
 *
 * Button circuit: BTN is pulled HIGH, pressing connects to GND
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "debug_uart.h"

volatile bool mcp_int_flag = false;

void mcp_int_isr() {
    mcp_int_flag = true;
}

bool mcp_write(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

uint8_t mcp_read(uint8_t reg) {
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(MCP23017_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

bool get_button() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    return (gpioa & 0x01) == 0;  // Active LOW (pressed = LOW)
}

void show_status() {
    debug_separator();
    debug_print("Button (GPA0): ");
    debug_println(get_button() ? "PRESSED" : "RELEASED");
    debug_separator();
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  r - Read button state");
    debug_println("  c - Continuous monitor (press any key to stop)");
    debug_println("  h - Show help");
    debug_println("");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("External Button Test");

    debug_println("Pin Configuration:");
    debug_println("  Button: MCP23017 GPA0");
    debug_print("  INTA: P1.01 (pin ");
    debug_print(MCP_INTA_PIN);
    debug_println(")");
    debug_println("");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Configure MCP23017
    // GPA0 = input, rest = output
    mcp_write(MCP_IODIRA, 0x01);

    // Enable pull-up on GPA0
    mcp_write(MCP_GPPUA, 0x01);

    // Enable interrupt on change for GPA0
    mcp_write(MCP_GPINTENA, 0x01);
    mcp_write(MCP_INTCONA, 0x00);  // Compare to previous value

    // Configure INTA as interrupt input
    pinMode(MCP_INTA_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MCP_INTA_PIN), mcp_int_isr, FALLING);

    // Clear any pending interrupts
    mcp_read(MCP_INTCAPA);

    debug_result("Button initialized", true);

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
            last_state = get_button();
        } else if (c == 'h' || c == 'H') {
            show_help();
        }
    }

    // Handle MCP23017 interrupt
    if (mcp_int_flag) {
        mcp_int_flag = false;

        // Read INTCAPA to clear interrupt and get state
        uint8_t intcap = mcp_read(MCP_INTCAPA);
        bool pressed = (intcap & 0x01) == 0;

        debug_print("[IRQ] Button: ");
        debug_println(pressed ? "PRESSED" : "RELEASED");
    }

    // Continuous polling mode
    if (continuous_mode) {
        bool current = get_button();
        if (current != last_state) {
            debug_print("Button: ");
            debug_println(current ? "PRESSED" : "RELEASED");
            last_state = current;
        }
        delay(20);  // Debounce
    }
}
