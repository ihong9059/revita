/*
 * Test 06: RS485 Communication Test
 *
 * Purpose: Test RS485 transceiver (MAX3485)
 * Pins:
 *   - UART1_TX (P0.20)
 *   - UART1_RX (P0.19)
 *   - DE (P1.04) - Transmit enable
 *   - RE# (P1.03) - Receive enable (active low)
 *
 * Note: This test uses USB CDC for debug (Serial) and Serial1 for RS485
 *
 * Test procedure:
 * 1. Connect A-B lines with termination resistor (or RS485 dongle)
 * 2. Send data and verify loopback
 *
 * DE/RE# control:
 *   - Transmit: DE=HIGH, RE#=HIGH
 *   - Receive:  DE=LOW,  RE#=LOW
 */

#include <Arduino.h>
#include "pin_config.h"

// Use USB CDC for debug output in this test
#define DEBUG_SERIAL Serial
#define RS485_SERIAL Serial1
#define RS485_BAUDRATE 9600

void rs485_tx_mode() {
    digitalWrite(RS485_DE_PIN, HIGH);
    digitalWrite(RS485_RE_PIN, HIGH);
    delayMicroseconds(10);
}

void rs485_rx_mode() {
    digitalWrite(RS485_DE_PIN, LOW);
    digitalWrite(RS485_RE_PIN, LOW);
    delayMicroseconds(10);
}

void debug_separator() {
    DEBUG_SERIAL.println("----------------------------------------");
}

void show_help() {
    DEBUG_SERIAL.println("");
    DEBUG_SERIAL.println("Commands:");
    DEBUG_SERIAL.println("  t - Send test message");
    DEBUG_SERIAL.println("  l - Loopback test");
    DEBUG_SERIAL.println("  s - Show pin status");
    DEBUG_SERIAL.println("  h - Show help");
    DEBUG_SERIAL.println("");
}

void show_status() {
    debug_separator();
    DEBUG_SERIAL.println("RS485 Pin Status:");
    DEBUG_SERIAL.print("  DE (P1.04):  ");
    DEBUG_SERIAL.println(digitalRead(RS485_DE_PIN) ? "HIGH (TX mode)" : "LOW (RX mode)");
    DEBUG_SERIAL.print("  RE# (P1.03): ");
    DEBUG_SERIAL.println(digitalRead(RS485_RE_PIN) ? "HIGH (RX disabled)" : "LOW (RX enabled)");
    debug_separator();
}

void send_test_message() {
    const char* msg = "Hello RS485!";

    DEBUG_SERIAL.print("Sending: ");
    DEBUG_SERIAL.println(msg);

    rs485_tx_mode();
    RS485_SERIAL.print(msg);
    RS485_SERIAL.flush();  // Wait for transmission complete
    rs485_rx_mode();

    DEBUG_SERIAL.println("Sent. Check connected device.");
}

void loopback_test() {
    const char* test_msg = "LOOPBACK_TEST";

    DEBUG_SERIAL.println("Loopback test (A-B must be connected)...");

    // Clear receive buffer
    while (RS485_SERIAL.available()) {
        RS485_SERIAL.read();
    }

    // Send test message
    rs485_tx_mode();
    RS485_SERIAL.print(test_msg);
    RS485_SERIAL.flush();
    rs485_rx_mode();

    // Wait for loopback
    delay(50);

    // Read response
    String received = "";
    unsigned long start = millis();
    while (millis() - start < 100) {
        if (RS485_SERIAL.available()) {
            received += (char)RS485_SERIAL.read();
        }
    }

    DEBUG_SERIAL.print("Sent:     ");
    DEBUG_SERIAL.println(test_msg);
    DEBUG_SERIAL.print("Received: ");
    DEBUG_SERIAL.println(received.c_str());

    if (received == test_msg) {
        DEBUG_SERIAL.println("  Loopback test: OK");
    } else {
        DEBUG_SERIAL.println("  Loopback test: FAIL");
        DEBUG_SERIAL.println("  Check A-B connection and termination.");
    }
}

void setup() {
    // Initialize USB CDC for debug
    DEBUG_SERIAL.begin(115200);
    delay(2000);  // Wait for USB enumeration

    DEBUG_SERIAL.println();
    debug_separator();
    DEBUG_SERIAL.println("TEST: RS485 Communication Test");
    DEBUG_SERIAL.println("NOTE: Debug via USB CDC, RS485 via Serial1");
    debug_separator();

    DEBUG_SERIAL.println("RS485 Configuration:");
    DEBUG_SERIAL.print("  TX Pin: P0.");
    DEBUG_SERIAL.println(UART1_TX_PIN);
    DEBUG_SERIAL.print("  RX Pin: P0.");
    DEBUG_SERIAL.println(UART1_RX_PIN);
    DEBUG_SERIAL.print("  DE Pin: P1.04 (pin ");
    DEBUG_SERIAL.print(RS485_DE_PIN);
    DEBUG_SERIAL.println(")");
    DEBUG_SERIAL.print("  RE# Pin: P1.03 (pin ");
    DEBUG_SERIAL.print(RS485_RE_PIN);
    DEBUG_SERIAL.println(")");
    DEBUG_SERIAL.print("  Baudrate: ");
    DEBUG_SERIAL.println(RS485_BAUDRATE);
    DEBUG_SERIAL.println("");

    // Initialize DE/RE pins
    pinMode(RS485_DE_PIN, OUTPUT);
    pinMode(RS485_RE_PIN, OUTPUT);

    // Start in receive mode
    rs485_rx_mode();

    // Initialize RS485 UART (Serial1)
    RS485_SERIAL.setPins(UART1_RX_PIN, UART1_TX_PIN);
    RS485_SERIAL.begin(RS485_BAUDRATE);

    DEBUG_SERIAL.println("  RS485 initialized: OK");

    show_status();
    show_help();
}

void loop() {
    // Check for debug commands
    if (DEBUG_SERIAL.available()) {
        char c = DEBUG_SERIAL.read();

        switch (c) {
            case 't':
            case 'T':
                send_test_message();
                break;

            case 'l':
            case 'L':
                loopback_test();
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

    // Check for RS485 received data
    if (RS485_SERIAL.available()) {
        DEBUG_SERIAL.print("RS485 RX: ");
        while (RS485_SERIAL.available()) {
            char c = RS485_SERIAL.read();
            DEBUG_SERIAL.print(c);
        }
        DEBUG_SERIAL.println("");
    }

    delay(10);
}
