/*
 * Test 02: I2C Bus Scan
 *
 * Purpose: Scan I2C bus and detect MCP23017
 * Pins: SDA(P0.13), SCL(P0.14)
 *
 * Expected devices:
 * - MCP23017 at 0x20
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "debug_uart.h"

void scan_i2c() {
    debug_println("Scanning I2C bus...");
    debug_println("");
    debug_println("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

    int device_count = 0;

    for (uint8_t row = 0; row < 8; row++) {
        debug_print("0x");
        debug_print(row);
        debug_print("0:");

        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = (row << 4) | col;

            if (addr < 0x03 || addr > 0x77) {
                debug_print("   ");
                continue;
            }

            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();

            if (error == 0) {
                debug_print(" ");
                debug_print_hex(addr);
                device_count++;
            } else {
                debug_print(" --");
            }
        }
        debug_println("");
    }

    debug_println("");
    debug_print("Found ");
    debug_print(device_count);
    debug_println(" device(s)");
    debug_separator();

    // Check for MCP23017
    debug_println("Checking MCP23017 (0x20)...");
    Wire.beginTransmission(MCP23017_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
        debug_result("MCP23017 detected", true);

        // Try to read IOCON register
        Wire.beginTransmission(MCP23017_ADDR);
        Wire.write(MCP_IOCON);
        Wire.endTransmission();

        Wire.requestFrom(MCP23017_ADDR, (uint8_t)1);
        if (Wire.available()) {
            uint8_t iocon = Wire.read();
            debug_print("  IOCON register: 0x");
            debug_print_hex(iocon);
            debug_println("");
        }
    } else {
        debug_result("MCP23017 detected", false);
        debug_print("  Error code: ");
        debug_println(error);
    }
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("I2C Bus Scan");

    debug_println("I2C Configuration:");
    debug_print("  SDA Pin: P0.");
    debug_println(I2C_SDA_PIN);
    debug_print("  SCL Pin: P0.");
    debug_println(I2C_SCL_PIN);
    debug_println("");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);  // 100kHz

    scan_i2c();

    debug_println("");
    debug_println("Press 's' to scan again");
}

void loop() {
    if (debug_available()) {
        char c = debug_read();
        if (c == 's' || c == 'S') {
            debug_println("");
            scan_i2c();
        }
    }
}
