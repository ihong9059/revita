/*
 * Test 12: Buzzer Control Test
 *
 * Purpose: Test buzzer via MCP23017
 * Pin: BUZZER_EN (MCP23017 GPB0)
 *
 * Note: Uses 12V power, so 12V step-up must be enabled first!
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

void set_buzzer(bool on) {
    uint8_t gpiob = mcp_read(MCP_GPIOB);
    if (on) {
        gpiob |= 0x01;  // GPB0 = HIGH
    } else {
        gpiob &= ~0x01; // GPB0 = LOW
    }
    mcp_write(MCP_GPIOB, gpiob);
}

bool get_buzzer() {
    return (mcp_read(MCP_GPIOB) & 0x01) != 0;
}

void set_12v_en(bool enable) {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    if (enable) {
        gpioa |= 0x80;  // GPA7 = HIGH
    } else {
        gpioa &= ~0x80; // GPA7 = LOW
    }
    mcp_write(MCP_GPIOA, gpioa);
}

bool get_12v_en() {
    return (mcp_read(MCP_GPIOA) & 0x80) != 0;
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  1 - Buzzer ON");
    debug_println("  0 - Buzzer OFF");
    debug_println("  t - Toggle buzzer");
    debug_println("  b - Beep pattern (3 short beeps)");
    debug_println("  p - Enable 12V power");
    debug_println("  s - Show status");
    debug_println("  h - Show help");
    debug_println("");
    debug_println("Note: 12V must be enabled for buzzer to work!");
    debug_println("");
}

void show_status() {
    debug_separator();
    debug_print("12V_EN (GPA7): ");
    debug_println(get_12v_en() ? "ON" : "OFF");
    debug_print("BUZZER (GPB0): ");
    debug_println(get_buzzer() ? "ON" : "OFF");
    debug_separator();
}

void beep_pattern() {
    debug_println("Playing beep pattern...");

    for (int i = 0; i < 3; i++) {
        set_buzzer(true);
        delay(100);
        set_buzzer(false);
        delay(100);
    }

    debug_println("Done!");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("Buzzer Control Test");

    debug_println("Pin Configuration:");
    debug_println("  BUZZER_EN: MCP23017 GPB0");
    debug_println("  12V_EN: MCP23017 GPA7");
    debug_println("  Buzzer frequency: 2.7kHz / 3kHz");
    debug_println("");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Configure MCP23017
    mcp_write(MCP_IODIRA, 0x01);  // GPA0 input, rest output
    mcp_write(MCP_IODIRB, 0x00);  // All output

    // Start with buzzer off, 12V off
    set_buzzer(false);
    // Don't turn off 12V by default - it might already be on

    debug_result("Buzzer initialized", true);

    show_status();
    show_help();
}

void loop() {
    if (debug_available()) {
        char c = debug_read();

        switch (c) {
            case '1':
                set_buzzer(true);
                debug_println("Buzzer: ON");
                break;

            case '0':
                set_buzzer(false);
                debug_println("Buzzer: OFF");
                break;

            case 't':
            case 'T':
                {
                    bool current = get_buzzer();
                    set_buzzer(!current);
                    debug_print("Buzzer toggled: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 'b':
            case 'B':
                beep_pattern();
                break;

            case 'p':
            case 'P':
                if (!get_12v_en()) {
                    set_12v_en(true);
                    debug_println("12V_EN: ON");
                    delay(100);  // Wait for 12V to stabilize
                } else {
                    debug_println("12V already enabled");
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
