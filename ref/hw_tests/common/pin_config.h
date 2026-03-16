/*
 * REVITA_LINK_v1 Pin Configuration
 * Common header for all test programs
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ============================================
// nRF52840 Direct Pins
// ============================================

// UART1 (RS485)
#define UART1_TX_PIN        20  // P0.20
#define UART1_RX_PIN        19  // P0.19
#define RS485_DE_PIN        36  // P1.04 (32 + 4)
#define RS485_RE_PIN        35  // P1.03 (32 + 3)

// UART2 (Debug) - All tests use this
#define UART2_TX_PIN        16  // P0.16
#define UART2_RX_PIN        15  // P0.15
#define DEBUG_BAUDRATE      115200

// I2C
#define I2C_SDA_PIN         13  // P0.13
#define I2C_SCL_PIN         14  // P0.14

// Digital Inputs (12V to 3.3V via optocoupler)
#define DIO_X_PIN           10  // P0.10
#define DIO_Y_PIN           9   // P0.09

// Vibration Sensor
#define VIB_SENSE_PIN       21  // P0.21

// Battery ADC (voltage divider: RDIV1=1M, RDIV2=1M, 1/2)
#define BAT_AIN_PIN         5   // P0.05 (AIN3)

// MCP23017 Interrupts
#define MCP_INTA_PIN        33  // P1.01 (32 + 1)
#define MCP_INTB_PIN        34  // P1.02 (32 + 2)

// QSPI Flash
#define QSPI_CS_PIN         26  // P0.26
#define QSPI_CLK_PIN        3   // P0.03
#define QSPI_DIO0_PIN       30  // P0.30
#define QSPI_DIO1_PIN       29  // P0.29
#define QSPI_DIO2_PIN       28  // P0.28
#define QSPI_DIO3_PIN       2   // P0.02

// ============================================
// MCP23017 GPIO Expander
// ============================================

#define MCP23017_ADDR       0x20

// Port A (GPA0-GPA7)
#define MCP_BTN             0   // GPA0 - Input
#define MCP_X_EN_P2         1   // GPA1 - Output
#define MCP_X_EN_B          2   // GPA2 - Output
#define MCP_X_EN_A          3   // GPA3 - Output
#define MCP_Y_EN_P2         4   // GPA4 - Output
#define MCP_Y_EN_B          5   // GPA5 - Output
#define MCP_Y_EN_A          6   // GPA6 - Output
#define MCP_12V_EN          7   // GPA7 - Output

// Port B (GPB0-GPB7)
#define MCP_BUZZER_EN       8   // GPB0 - Output (8 = Port B offset)
#define MCP_LED_EN          15  // GPB7 - Output (8 + 7)

// ============================================
// MCP23017 Register Addresses
// ============================================

#define MCP_IODIRA          0x00
#define MCP_IODIRB          0x01
#define MCP_IPOLA           0x02
#define MCP_IPOLB           0x03
#define MCP_GPINTENA        0x04
#define MCP_GPINTENB        0x05
#define MCP_DEFVALA         0x06
#define MCP_DEFVALB         0x07
#define MCP_INTCONA         0x08
#define MCP_INTCONB         0x09
#define MCP_IOCON           0x0A
#define MCP_GPPUA           0x0C
#define MCP_GPPUB           0x0D
#define MCP_INTFA           0x0E
#define MCP_INTFB           0x0F
#define MCP_INTCAPA         0x10
#define MCP_INTCAPB         0x11
#define MCP_GPIOA           0x12
#define MCP_GPIOB           0x13
#define MCP_OLATA           0x14
#define MCP_OLATB           0x15

#endif // PIN_CONFIG_H
