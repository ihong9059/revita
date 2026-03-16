/*
 * Test 01: UART2 Debug Test
 *
 * Purpose: Verify UART2 debugging output and input
 * Pins: TX(P0.16), RX(P0.15)
 *
 * Test procedure:
 * 1. Connect USB-UART adapter to UART2 pins
 * 2. Open serial terminal at 115200 baud
 * 3. Should see "Hello REVITA_LINK!" message
 * 4. Type characters to test echo
 */

#include <Arduino.h>
#include "pin_config.h"
#include "debug_uart.h"

// RAK4630 built-in LED (directly toggle for debug)
#define LED_BUILTIN_PIN 35  // P1.03 - Green LED on RAK4630

void setup() {
    // Initialize LED for visual feedback
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);  // LED ON

    // Initialize debug UART
    debug_init();

    delay(1000);  // Wait for serial connection

    digitalWrite(LED_BUILTIN_PIN, HIGH);  // LED OFF
    delay(200);
    digitalWrite(LED_BUILTIN_PIN, LOW);   // LED ON

    debug_test_header("UART2 Debug Test");

    debug_println("Hello REVITA_LINK!");
    debug_println("");
    debug_println("UART2 Configuration:");
    debug_print("  TX Pin: P0.");
    debug_println(UART2_TX_PIN);
    debug_print("  RX Pin: P0.");
    debug_println(UART2_RX_PIN);
    debug_print("  Baudrate: ");
    debug_println(DEBUG_BAUDRATE);
    debug_println("");
    debug_println("Echo test - type characters:");
    debug_separator();
}

void loop() {
    // Echo test: send back received characters
    if (debug_available()) {
        char c = debug_read();

        // Echo the character
        DEBUG_SERIAL.print("Received: '");
        DEBUG_SERIAL.print(c);
        DEBUG_SERIAL.print("' (0x");
        debug_print_hex((uint8_t)c);
        DEBUG_SERIAL.println(")");

        // Special commands
        if (c == 'h' || c == 'H') {
            debug_println("");
            debug_println("Commands:");
            debug_println("  h - Show this help");
            debug_println("  t - Print timestamp");
            debug_println("  r - Reboot");
            debug_println("");
        }
        else if (c == 't' || c == 'T') {
            debug_print("Uptime: ");
            debug_print((int)(millis() / 1000));
            debug_println(" seconds");
        }
        else if (c == 'r' || c == 'R') {
            debug_println("Rebooting...");
            delay(100);
            NVIC_SystemReset();
        }
    }
}
