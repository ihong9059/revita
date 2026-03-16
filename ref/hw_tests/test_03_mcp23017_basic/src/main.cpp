/*
 * Test 03: MCP23017 Basic Test
 *
 * Purpose: Test MCP23017 GPIO expander basic functions
 * Pins: I2C (SDA: P0.13, SCL: P0.14)
 *
 * Test procedure:
 * 1. Initialize MCP23017
 * 2. Configure port directions
 * 3. Test output toggling
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "debug_uart.h"

// Write to MCP23017 register
bool mcp_write(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

// Read from MCP23017 register
uint8_t mcp_read(uint8_t reg) {
    Wire.beginTransmission(MCP23017_ADDR);
    Wire.write(reg);
    Wire.endTransmission();

    Wire.requestFrom(MCP23017_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

// Initialize MCP23017
bool mcp_init() {
    // Port A: GPA0 input (BTN), GPA1-7 output
    // IODIRA: bit=1 is input, bit=0 is output
    // GPA0 = BTN (input) = 0x01
    if (!mcp_write(MCP_IODIRA, 0x01)) return false;

    // Port B: GPB0 output (BUZZER), GPB7 output (LED), others not used
    // All outputs = 0x00
    if (!mcp_write(MCP_IODIRB, 0x00)) return false;

    // Enable pull-up on BTN (GPA0)
    if (!mcp_write(MCP_GPPUA, 0x01)) return false;

    // Initialize outputs to LOW
    if (!mcp_write(MCP_GPIOA, 0x00)) return false;
    if (!mcp_write(MCP_GPIOB, 0x00)) return false;

    return true;
}

// Set output pin (0-7 for Port A, 8-15 for Port B)
void mcp_set_pin(uint8_t pin, bool value) {
    uint8_t port_reg;
    uint8_t pin_bit;

    if (pin < 8) {
        port_reg = MCP_GPIOA;
        pin_bit = pin;
    } else {
        port_reg = MCP_GPIOB;
        pin_bit = pin - 8;
    }

    uint8_t current = mcp_read(port_reg);
    if (value) {
        current |= (1 << pin_bit);
    } else {
        current &= ~(1 << pin_bit);
    }
    mcp_write(port_reg, current);
}

// Read input pin
bool mcp_get_pin(uint8_t pin) {
    uint8_t port_reg;
    uint8_t pin_bit;

    if (pin < 8) {
        port_reg = MCP_GPIOA;
        pin_bit = pin;
    } else {
        port_reg = MCP_GPIOB;
        pin_bit = pin - 8;
    }

    uint8_t value = mcp_read(port_reg);
    return (value & (1 << pin_bit)) != 0;
}

void print_status() {
    debug_separator();
    debug_println("MCP23017 Status:");

    uint8_t gpioa = mcp_read(MCP_GPIOA);
    uint8_t gpiob = mcp_read(MCP_GPIOB);

    debug_print("  GPIOA: 0x");
    debug_print_hex(gpioa);
    debug_print(" (BTN=");
    debug_print(mcp_get_pin(MCP_BTN) ? "HIGH" : "LOW");
    debug_println(")");

    debug_print("  GPIOB: 0x");
    debug_print_hex(gpiob);
    debug_println("");

    debug_println("");
    debug_println("Output States:");
    debug_print("  12V_EN(GPA7): ");
    debug_println((gpioa & 0x80) ? "ON" : "OFF");
    debug_print("  Y_EN_A(GPA6): ");
    debug_println((gpioa & 0x40) ? "ON" : "OFF");
    debug_print("  Y_EN_B(GPA5): ");
    debug_println((gpioa & 0x20) ? "ON" : "OFF");
    debug_print("  Y_EN_P2(GPA4): ");
    debug_println((gpioa & 0x10) ? "ON" : "OFF");
    debug_print("  X_EN_A(GPA3): ");
    debug_println((gpioa & 0x08) ? "ON" : "OFF");
    debug_print("  X_EN_B(GPA2): ");
    debug_println((gpioa & 0x04) ? "ON" : "OFF");
    debug_print("  X_EN_P2(GPA1): ");
    debug_println((gpioa & 0x02) ? "ON" : "OFF");
    debug_print("  BUZZER(GPB0): ");
    debug_println((gpiob & 0x01) ? "ON" : "OFF");
    debug_print("  LED(GPB7): ");
    debug_println((gpiob & 0x80) ? "ON" : "OFF");
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  s - Show status");
    debug_println("  1 - Toggle 12V_EN");
    debug_println("  l - Toggle LED");
    debug_println("  b - Toggle BUZZER");
    debug_println("  a - All outputs ON");
    debug_println("  o - All outputs OFF");
    debug_println("  h - Show help");
    debug_println("");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("MCP23017 Basic Test");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Initialize MCP23017
    debug_println("Initializing MCP23017...");
    if (mcp_init()) {
        debug_result("MCP23017 init", true);
    } else {
        debug_result("MCP23017 init", false);
        debug_println("ERROR: Cannot communicate with MCP23017!");
        while(1) { delay(1000); }
    }

    print_status();
    show_help();
}

void loop() {
    if (debug_available()) {
        char c = debug_read();

        switch (c) {
            case 's':
            case 'S':
                print_status();
                break;

            case '1':
                {
                    bool current = mcp_get_pin(MCP_12V_EN);
                    mcp_set_pin(MCP_12V_EN, !current);
                    debug_print("12V_EN: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 'l':
            case 'L':
                {
                    bool current = mcp_get_pin(MCP_LED_EN);
                    mcp_set_pin(MCP_LED_EN, !current);
                    debug_print("LED: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 'b':
            case 'B':
                {
                    bool current = mcp_get_pin(MCP_BUZZER_EN);
                    mcp_set_pin(MCP_BUZZER_EN, !current);
                    debug_print("BUZZER: ");
                    debug_println(!current ? "ON" : "OFF");
                }
                break;

            case 'a':
            case 'A':
                mcp_write(MCP_GPIOA, 0xFE);  // All except BTN
                mcp_write(MCP_GPIOB, 0x81);  // BUZZER + LED
                debug_println("All outputs ON");
                break;

            case 'o':
            case 'O':
                mcp_write(MCP_GPIOA, 0x00);
                mcp_write(MCP_GPIOB, 0x00);
                debug_println("All outputs OFF");
                break;

            case 'h':
            case 'H':
                show_help();
                break;
        }
    }

    delay(10);
}
