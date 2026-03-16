/*
 * REVITA_LINK_v1 Variant
 * Based on RAK4630 (nRF52840 + SX1262)
 * Custom QSPI pins for MX25R1635F Flash
 */

#ifndef _VARIANT_REVITA_LINK_
#define _VARIANT_REVITA_LINK_

/** Master clock frequency */
#define VARIANT_MCK       (64000000ul)

#define USE_LFXO      // Board uses 32khz crystal for LF

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Number of pins defined in PinDescription array
#define PINS_COUNT           (48)
#define NUM_DIGITAL_PINS     (48)
#define NUM_ANALOG_INPUTS    (6)
#define NUM_ANALOG_OUTPUTS   (0)

// LEDs - no onboard LEDs, controlled via MCP23017
#define PIN_LED1             (0xFF)  // Not used
#define LED_BUILTIN          PIN_LED1
#define LED_STATE_ON         1

/*
 * Analog pins
 */
#define PIN_A0               (2)   // P0.02/AIN0
#define PIN_A1               (3)   // P0.03/AIN1
#define PIN_A2               (4)   // P0.04/AIN2
#define PIN_A3               (5)   // P0.05/AIN3 - BAT_AIN
#define PIN_A4               (28)  // P0.28/AIN4
#define PIN_A5               (29)  // P0.29/AIN5
#define PIN_A6               (30)  // P0.30/AIN6
#define PIN_A7               (31)  // P0.31/AIN7

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6;
static const uint8_t A7  = PIN_A7;
#define ADC_RESOLUTION    14

// Battery ADC
#define PIN_VBAT           (5)  // P0.05/AIN3

/*
 * Serial interfaces
 */
// UART1 (RS485)
#define PIN_SERIAL1_RX       (19)  // P0.19
#define PIN_SERIAL1_TX       (20)  // P0.20

// UART2 (Debug)
#define PIN_SERIAL2_RX       (15)  // P0.15
#define PIN_SERIAL2_TX       (16)  // P0.16

/*
 * SPI Interfaces - Not used, QSPI only
 */
#define SPI_INTERFACES_COUNT 0

/*
 * Wire Interfaces (I2C)
 */
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA         (13)  // P0.13
#define PIN_WIRE_SCL         (14)  // P0.14

/*
 * QSPI Pins - MX25R1635F Flash
 * Based on REVITA_LINK_v1 schematic
 */
#define PIN_QSPI_SCK         3   // P0.03
#define PIN_QSPI_CS          26  // P0.26
#define PIN_QSPI_IO0         30  // P0.30
#define PIN_QSPI_IO1         29  // P0.29
#define PIN_QSPI_IO2         28  // P0.28
#define PIN_QSPI_IO3         2   // P0.02

// On-board QSPI Flash
#define EXTERNAL_FLASH_DEVICES   MX25R1635F
#define EXTERNAL_FLASH_USE_QSPI

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
