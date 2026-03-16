/*
 * Test 11: LED Control Test
 *
 * Purpose: Test external LED via MCP23017
 * Pin: LED_EN (MCP23017 GPB7)
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

void set_led(bool on) {
    uint8_t gpiob = mcp_read(MCP_GPIOB);
    if (on) {
        gpiob |= 0x80;  // GPB7 = HIGH
    } else {
        gpiob &= ~0x80; // GPB7 = LOW
    }
    mcp_write(MCP_GPIOB, gpiob);
}

bool get_led() {
    return (mcp_read(MCP_GPIOB) & 0x80) != 0;
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  1 - LED ON");
    debug_println("  0 - LED OFF");
    debug_println("  t - Toggle LED");
    debug_println("  b - Blink test (5 times)");
    debug_println("  s - Show status");
    debug_println("  h - Show help");
    debug_println("");
}

void show_status() {
    debug_separator();
    debug_print("LED (GPB7): ");
    debug_println(get_led() ? "ON" : "OFF");
    debug_separator();
}

void blink_test(int count, int delay_ms) {
    debug_print("Blinking ");
    debug_print(count);
    debug_println(" times...");

    for (int i = 0; i < count; i++) {
        set_led(true);
        delay(delay_ms);
        set_led(false);
        delay(delay_ms);
        debug_print(".");
    }
    debug_println(" Done!");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("LED Control Test");

    debug_println("Pin Configuration:");
    debug_println("  LED_EN: MCP23017 GPB7");
    debug_println("");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Configure MCP23017
    mcp_write(MCP_IODIRA, 0x01);  // GPA0 input, rest output
    mcp_write(MCP_IODIRB, 0x00);  // All output

    // Start with LED off
    set_led(false);

    debug_result("LED initialized", true);

    show_status();
    show_help();
}

void loop() {
    if (debug_available()) {
        char c = debug_read();

        switch (c) {
            case '1':
                set_led(true);
                debug_println("LED: ON");
                break;

            case '0':
                set_led(false);
                debug_println("LED: OFF");
                break;

            case 't':
            case 'T':
                {
                    bool current = get_led();
                    set_led(!current);
                    debug_print("LED toggled: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 'b':
            case 'B':
                blink_test(5, 200);
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
