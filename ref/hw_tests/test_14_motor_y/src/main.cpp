/*
 * Test 14: Motor Y Control Test
 *
 * Purpose: Test 3-Line motor control for Y axis
 * Pins (MCP23017):
 *   - Y_EN_A:  GPA6 - A direction enable
 *   - Y_EN_B:  GPA5 - B direction enable
 *   - Y_EN_P2: GPA4 - Common power switch
 *
 * Motor control logic:
 *   - Forward:  EN_P2=HIGH, EN_A=HIGH, EN_B=LOW
 *   - Reverse:  EN_P2=HIGH, EN_A=LOW,  EN_B=HIGH
 *   - Stop:     EN_P2=LOW  or  EN_A=EN_B=LOW
 *   - Brake:    EN_P2=HIGH, EN_A=HIGH, EN_B=HIGH
 *
 * WARNING: 12V must be enabled first!
 */

#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"
#include "debug_uart.h"

// Motor control bits in GPIOA
#define MOTOR_Y_P2_BIT  (1 << 4)  // GPA4
#define MOTOR_Y_B_BIT   (1 << 5)  // GPA5
#define MOTOR_Y_A_BIT   (1 << 6)  // GPA6
#define MOTOR_Y_MASK    (MOTOR_Y_P2_BIT | MOTOR_Y_B_BIT | MOTOR_Y_A_BIT)

#define EN_12V_BIT      (1 << 7)  // GPA7

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

void motor_y_stop() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    gpioa &= ~MOTOR_Y_MASK;  // Clear all motor bits
    mcp_write(MCP_GPIOA, gpioa);
}

void motor_y_forward() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    gpioa &= ~MOTOR_Y_MASK;
    gpioa |= MOTOR_Y_P2_BIT | MOTOR_Y_A_BIT;  // P2=1, A=1, B=0
    mcp_write(MCP_GPIOA, gpioa);
}

void motor_y_reverse() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    gpioa &= ~MOTOR_Y_MASK;
    gpioa |= MOTOR_Y_P2_BIT | MOTOR_Y_B_BIT;  // P2=1, A=0, B=1
    mcp_write(MCP_GPIOA, gpioa);
}

void motor_y_brake() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    gpioa &= ~MOTOR_Y_MASK;
    gpioa |= MOTOR_Y_P2_BIT | MOTOR_Y_A_BIT | MOTOR_Y_B_BIT;  // P2=1, A=1, B=1
    mcp_write(MCP_GPIOA, gpioa);
}

void set_12v_en(bool enable) {
    uint8_t gpioa = mcp_read(MCP_GPIOA);
    if (enable) {
        gpioa |= EN_12V_BIT;
    } else {
        gpioa &= ~EN_12V_BIT;
    }
    mcp_write(MCP_GPIOA, gpioa);
}

bool get_12v_en() {
    return (mcp_read(MCP_GPIOA) & EN_12V_BIT) != 0;
}

void show_status() {
    uint8_t gpioa = mcp_read(MCP_GPIOA);

    debug_separator();
    debug_println("Motor Y Status:");
    debug_print("  12V_EN (GPA7): ");
    debug_println((gpioa & EN_12V_BIT) ? "ON" : "OFF");
    debug_print("  Y_EN_A (GPA6): ");
    debug_println((gpioa & MOTOR_Y_A_BIT) ? "HIGH" : "LOW");
    debug_print("  Y_EN_B (GPA5): ");
    debug_println((gpioa & MOTOR_Y_B_BIT) ? "HIGH" : "LOW");
    debug_print("  Y_EN_P2 (GPA4): ");
    debug_println((gpioa & MOTOR_Y_P2_BIT) ? "HIGH" : "LOW");

    debug_print("  Motor state: ");
    bool p2 = gpioa & MOTOR_Y_P2_BIT;
    bool a = gpioa & MOTOR_Y_A_BIT;
    bool b = gpioa & MOTOR_Y_B_BIT;

    if (!p2) {
        debug_println("STOP (power off)");
    } else if (a && !b) {
        debug_println("FORWARD");
    } else if (!a && b) {
        debug_println("REVERSE");
    } else if (a && b) {
        debug_println("BRAKE");
    } else {
        debug_println("STOP");
    }

    debug_separator();
}

void show_help() {
    debug_println("");
    debug_println("Commands:");
    debug_println("  f - Forward");
    debug_println("  r - Reverse");
    debug_println("  s - Stop");
    debug_println("  b - Brake");
    debug_println("  p - Enable 12V power");
    debug_println("  o - Disable 12V power");
    debug_println("  t - Test sequence (fwd-stop-rev-stop)");
    debug_println("  ? - Show status");
    debug_println("  h - Show help");
    debug_println("");
    debug_println("WARNING: 12V must be enabled for motor to work!");
    debug_println("");
}

void test_sequence() {
    debug_println("Running test sequence...");

    if (!get_12v_en()) {
        debug_println("Enabling 12V...");
        set_12v_en(true);
        delay(200);
    }

    debug_println("Forward 1 second...");
    motor_y_forward();
    delay(1000);

    debug_println("Stop 500ms...");
    motor_y_stop();
    delay(500);

    debug_println("Reverse 1 second...");
    motor_y_reverse();
    delay(1000);

    debug_println("Stop.");
    motor_y_stop();

    debug_println("Test sequence complete!");
}

void setup() {
    debug_init();
    delay(1000);

    debug_test_header("Motor Y Control Test");

    debug_println("Pin Configuration:");
    debug_println("  Y_EN_A:  MCP23017 GPA6");
    debug_println("  Y_EN_B:  MCP23017 GPA5");
    debug_println("  Y_EN_P2: MCP23017 GPA4");
    debug_println("  12V_EN:  MCP23017 GPA7");
    debug_println("");

    // Initialize I2C
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(100000);

    // Configure MCP23017
    mcp_write(MCP_IODIRA, 0x01);  // GPA0 input, rest output
    mcp_write(MCP_IODIRB, 0x00);  // All output

    // Start with motor stopped
    motor_y_stop();

    debug_result("Motor Y initialized", true);

    show_status();
    show_help();
}

void loop() {
    if (debug_available()) {
        char c = debug_read();

        switch (c) {
            case 'f':
            case 'F':
                motor_y_forward();
                debug_println("Motor Y: FORWARD");
                break;

            case 'r':
            case 'R':
                motor_y_reverse();
                debug_println("Motor Y: REVERSE");
                break;

            case 's':
            case 'S':
                motor_y_stop();
                debug_println("Motor Y: STOP");
                break;

            case 'b':
            case 'B':
                motor_y_brake();
                debug_println("Motor Y: BRAKE");
                break;

            case 'p':
            case 'P':
                set_12v_en(true);
                debug_println("12V: ON");
                break;

            case 'o':
            case 'O':
                motor_y_stop();
                set_12v_en(false);
                debug_println("12V: OFF, Motor stopped");
                break;

            case 't':
            case 'T':
                test_sequence();
                break;

            case '?':
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
