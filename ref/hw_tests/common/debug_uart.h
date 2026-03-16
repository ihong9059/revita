/*
 * REVITA_LINK_v1 Debug UART Helper
 * Uses UART2 (TX: P0.16, RX: P0.15)
 */

#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include <Arduino.h>
#include "pin_config.h"

// Use Serial1 for debug output on RAK4630
#define DEBUG_SERIAL Serial1

// Initialize debug UART
inline void debug_init() {
    DEBUG_SERIAL.setPins(UART2_RX_PIN, UART2_TX_PIN);
    DEBUG_SERIAL.begin(DEBUG_BAUDRATE);
    delay(100);
}

// Print functions
inline void debug_print(const char* msg) {
    DEBUG_SERIAL.print(msg);
}

inline void debug_println(const char* msg) {
    DEBUG_SERIAL.println(msg);
}

inline void debug_print(int val) {
    DEBUG_SERIAL.print(val);
}

inline void debug_println(int val) {
    DEBUG_SERIAL.println(val);
}

inline void debug_print(float val, int decimals = 2) {
    DEBUG_SERIAL.print(val, decimals);
}

inline void debug_println(float val, int decimals = 2) {
    DEBUG_SERIAL.println(val, decimals);
}

inline void debug_print_hex(uint8_t val) {
    if (val < 16) DEBUG_SERIAL.print("0");
    DEBUG_SERIAL.print(val, HEX);
}

// Print separator line
inline void debug_separator() {
    DEBUG_SERIAL.println("----------------------------------------");
}

// Print test header
inline void debug_test_header(const char* test_name) {
    DEBUG_SERIAL.println();
    debug_separator();
    DEBUG_SERIAL.print("TEST: ");
    DEBUG_SERIAL.println(test_name);
    debug_separator();
}

// Print OK/FAIL result
inline void debug_result(const char* item, bool success) {
    DEBUG_SERIAL.print("  ");
    DEBUG_SERIAL.print(item);
    DEBUG_SERIAL.print(": ");
    DEBUG_SERIAL.println(success ? "OK" : "FAIL");
}

// Check if data available
inline bool debug_available() {
    return DEBUG_SERIAL.available() > 0;
}

// Read character
inline char debug_read() {
    return DEBUG_SERIAL.read();
}

// Read line (blocking)
inline String debug_readLine() {
    String line = "";
    while (true) {
        if (DEBUG_SERIAL.available()) {
            char c = DEBUG_SERIAL.read();
            if (c == '\n' || c == '\r') {
                if (line.length() > 0) return line;
            } else {
                line += c;
            }
        }
    }
}

// Wait for any key press
inline void debug_wait_key(const char* prompt = "Press any key to continue...") {
    DEBUG_SERIAL.println(prompt);
    while (!DEBUG_SERIAL.available()) {
        delay(10);
    }
    while (DEBUG_SERIAL.available()) {
        DEBUG_SERIAL.read();
    }
}

#endif // DEBUG_UART_H
