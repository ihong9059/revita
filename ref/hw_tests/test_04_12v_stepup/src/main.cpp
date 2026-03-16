/*
 * Test 04: 12V Step-up Converter Test
 *
 * Purpose: Test 12V boost converter (TPS61178)
 * Control Pin: 12V_EN (MCP23017 GPA7)
 *
 * Test procedure:
 * 1. Measure 12V_VOUT with multimeter
 * 2. Toggle 12V_EN and verify output voltage
 *
 * WARNING: Ensure proper load or no-load operation!
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "debug_uart.h"

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

void set_12v_en(bool enable) {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    if (enable) {
        gpioa |= (1 << 7);  // GPA7 = HIGH
    } else {
        gpioa &= ~(1 << 7); // GPA7 = LOW
    }
    mcp_write(MCP_GPIOA, gpioa);
}

bool get_12v_en() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    return (gpioa & 0x80) != 0;
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  1 - Enable 12V (ON)");
    debug_println("  0 - Disable 12V (OFF)");
    debug_println("  t - Toggle 12V");
    debug_println("  s - Show status");
    debug_println("  h - Show help");
    debug_println("");
    debug_println("WARNING: Measure 12V_VOUT with multimeter!");
    debug_println("");
}

void show_status() {
    debug_separator();
    debug_print("12V_EN (GPA7): ");
    debug_println(get_12v_en() ? "ON (HIGH)" : "OFF (LOW)");
    debug_println("");
    debug_println("Expected output:");
    debug_println("  12V_EN=HIGH -> 12V_VOUT ~ 12V");
    debug_println("  12V_EN=LOW  -> 12V_VOUT ~ 0V");
    debug_separator();
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("12V Step-up Converter Test");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Configure MCP23017
    debug_println("Initializing MCP23017...");

    // Set GPA7 as output, GPA0 as input
    mcp_write(MCP_IODIRA, 0x01);
    mcp_write(MCP_IODIRB, 0x00);

    // Start with 12V OFF for safety
    set_12v_en(false);

    debug_result("MCP23017 ready", true);
    debug_println("");
    debug_println("12V is initially OFF for safety.");

    show_status();
    show_help();
}

void loop() {
    if (debug_available()) {
        char c = debug_read();

        switch (c) {
            case '1':
                set_12v_en(true);
                debug_println("12V_EN: ON");
                debug_println(">>> Measure 12V_VOUT - should be ~12V");
                break;

            case '0':
                set_12v_en(false);
                debug_println("12V_EN: OFF");
                debug_println(">>> Measure 12V_VOUT - should be ~0V");
                break;

            case 't':
            case 'T':
                {
                    bool current = get_12v_en();
                    set_12v_en(!current);
                    debug_print("12V_EN toggled: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 's':
            case 'S':
                show_status();
                break;

            case 'h':
            case 'H':
                show_help();
                break;
        }
    }

    delay(10);
}
