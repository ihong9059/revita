#ifndef TOWER_PORT_H
#define TOWER_PORT_H

/*
 * REVITA TOWER V1 - Port Assignment
 *
 * MCU: nRF52840 (RAK4631)
 * I/O Expander: MCP23017 (I2C 0x20)
 * UART MUX: TMUX1574PWR (4ch 2:1, SEL 1pin)
 */

/* ================================================================
 *  MCU Direct GPIO (nRF52840)
 * ================================================================ */

/* I2C Bus */
#define PIN_I2C_SDA             13      /* P0.13 */
#define PIN_I2C_SCL             14      /* P0.14 */

/* UART0 - RS485 (via TMUX1574 MUX) */
#define PIN_UART0_RX            19      /* P0.19  RAK_UART1_RX → MUX COM_RX */
#define PIN_UART0_TX            20      /* P0.20  RAK_UART1_TX → MUX COM_TX */

/* UART1 - Debug */
#define PIN_UART1_RX            15      /* P0.15  RAK_UART2_RX */
#define PIN_UART1_TX            16      /* P0.16  RAK_UART2_TX */

/* RS485 Direction Control */
#define PIN_RS485_DE            17      /* P0.17  DE:  HIGH=TX enable */
#define PIN_RS485_RE_N          21      /* P0.21  RE#: LOW=RX enable */

/* SPI (RAK4631 LoRa - SX1262) */
#define PIN_SPI_CLK             3       /* P0.03 */
#define PIN_SPI_CS              26      /* P0.26 */
#define PIN_SPI_MISO            29      /* P0.29 */
#define PIN_SPI_MOSI            30      /* P0.30 */

/* ADC */
#define PIN_BATT_ADC            5       /* P0.05  배터리 전압 확인 (AIN0) */
#define PIN_AIN1                31      /* P0.31 */

/* LED */
#define PIN_LED1                35      /* P1.03  Green */

/* Sensor */
#define PIN_VIB_SENSOR          36      /* P1.04  진동 센서 Digital Input */

/* Button */
#define PIN_BUTTON              34      /* P1.02  버튼 입력 */

/* USB (미확인 - 검토사항 참조) */
/* MCU_USB_D_N  - 핀 미확인 */
/* MCU_USB_D_P  - 핀 미확인 */

/* LTE 제어 (미확인 - 검토사항 참조) */
/* LTE_PW_ON   - 핀 미확인 */
/* LTE_DTR     - 핀 미확인 */
/* LTE_RI      - 핀 미확인 */

/* 기타 미확인 */
/* P0.26 UARPC2  - 용도 미확인 (SPI_CS와 공유?) */
/* P1.01 SIO_35  - 용도 미확인 */

/* ================================================================
 *  MCP23017 I2C I/O Expander (0x20)
 * ================================================================ */

/* MCP23017 Registers */
#define MCP_REG_IODIRA          0x00
#define MCP_REG_IODIRB          0x01
#define MCP_REG_OLATA           0x14
#define MCP_REG_OLATB           0x15

/* GPA Port */
#define MCP_GPA0_LTE_RST_N     (1 << 0)    /* LTE 모듈 리셋 (Active Low) */
#define MCP_GPA1                (1 << 1)    /* 공란 - 미할당 */
#define MCP_GPA2_12V_EN         (1 << 2)    /* 12V 전원 활성화 */
#define MCP_GPA3_MUX_SEL        (1 << 3)    /* TMUX1574 SEL: LOW=RS485, HIGH=SBC */
#define MCP_GPA4_MUX_EN_N       (1 << 4)    /* TMUX1574 EN#: LOW=Enable */
#define MCP_GPA5_3V3_EN         (1 << 5)    /* 3.3V 전원 활성화 */
#define MCP_GPA6_5V_EN          (1 << 6)    /* 5V 전원 활성화 */
#define MCP_GPA7_CAM_EN         (1 << 7)    /* 카메라 활성화 */

/* GPB Port */
#define MCP_GPB0_SBC_RST_N      (1 << 0)    /* SBC 리셋 (Active Low) */
#define MCP_GPB1_BUZZER_EN      (1 << 1)    /* 부저 활성화 */
#define MCP_GPB2                (1 << 2)    /* 공란 - 미할당 */
#define MCP_GPB3                (1 << 3)    /* 공란 - 미할당 */
#define MCP_GPB4                (1 << 4)    /* 공란 - 미할당 */
#define MCP_GPB5                (1 << 5)    /* 공란 - 미할당 */
#define MCP_GPB6_LED_EN         (1 << 6)    /* LED 활성화 */
#define MCP_GPB7                (1 << 7)    /* 공란 - 미할당 */

#endif /* TOWER_PORT_H */
