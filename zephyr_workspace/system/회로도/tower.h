/**
 * @file    tower.h
 * @brief   REVITA TOWER 보드 핀 매핑 헤더 (RAK4630 + MCP23017 I/O Expander)
 * @details U8 = RAK4630-9-SM-I (nRF52840), U2 = MCP23017-E/SS, U15 = MX25R1635 QSPI Flash.
 *          RAK4630의 GPIO 가용 핀이 부족하여 보드 전원 / 리셋 / MUX 등의 보조 신호는
 *          MCP23017 (16-bit I2C I/O Expander)를 거쳐 제어한다.
 *
 *  ┌──────────────┐                  ┌──────────────┐
 *  │  RAK4630     │  I2C(P0.13/14)   │  MCP23017    │── GPA0..7  (전원 / LTE / MUX / CAM)
 *  │  (nRF52840)  │ ───────────────► │  @ 0x20      │── GPB0..7  (SBC RST / BUZZER / LED)
 *  └──────────────┘                  └──────────────┘
 *
 *  MCU 직접 핀
 *  ──────────────────────────────────────────────────────
 *   4  P0.13/I2C_SDA   → I2C_SDA   (MCP23017 공유)
 *   5  P0.14/I2C_SCL   → I2C_SCL   (MCP23017 공유)
 *   6  P0.15/UART2_RX  → RAK_UART2_RX
 *   7  P0.16/UART2_TX  → RAK_UART2_TX
 *   8  P0.17/UART2_DE  → RAK_DE   (RS-485 송신 enable)
 *   9  P0.19/UART1_RX  → RAK_UART1_RX  (LTE 모뎀)
 *  10  P0.20/UART1_TX  → RAK_UART1_TX  (LTE 모뎀)
 *  11  P0.21/UART1_DE  → RAK_RE#  (RS-485 수신 enable, active low)
 *  12  P0.10/NFC2      → LTE_RI   (LTE Ring Indicator, NFC→GPIO 재설정 필요)
 *  13  P0.09/NFC1      → LTE_DTR  (LTE Data Terminal Ready)
 *  17  NRF_RESET       → RST#    (R31 470k 풀업)
 *  18  SWDCLK          → SWCLK
 *  19  SWDIO           → SWDIO
 *  24  P1.02/SW2       → BTN     (사용자 버튼)
 *  28  P1.04/LED2      → VIB_SENSE (진동 감지 입력)
 *  29..34  QSPI        → 외부 NOR Flash U15 (MX25R1635FZUIL0)
 *  40  P0.05/AIN3      → BAT_AIN (배터리 ADC)
 */

#ifndef TOWER_H_
#define TOWER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  nRF52840 GPIO 매크로
 * ============================================================ */
#ifndef NRF_GPIO_PIN_MAP
#define NRF_GPIO_PIN_MAP(port, pin)   (((port) << 5) | ((pin) & 0x1F))
#endif

/* ============================================================
 *  PART 1. RAK4630 직접 핀
 * ============================================================ */

/* I2C0 — MCP23017과 공유, 추후 다른 I2C 슬레이브도 동일 버스 사용 */
#define PIN_I2C_SDA            NRF_GPIO_PIN_MAP(0, 13)   /* P0.13, 모듈핀 4  */
#define PIN_I2C_SCL            NRF_GPIO_PIN_MAP(0, 14)   /* P0.14, 모듈핀 5  */
#define TOWER_I2C_FREQ_HZ      400000U                   /* 400kHz Fast Mode */

/* UART1 — LTE 모뎀 (RC76xx 등) */
#define PIN_LTE_UART_RX        NRF_GPIO_PIN_MAP(0, 19)   /* P0.19, 모듈핀 9  */
#define PIN_LTE_UART_TX        NRF_GPIO_PIN_MAP(0, 20)   /* P0.20, 모듈핀 10 */
#define PIN_LTE_RI             NRF_GPIO_PIN_MAP(0, 10)   /* P0.10/NFC2, 모듈핀 12 (UICR.NFCPINS off) */
#define PIN_LTE_DTR            NRF_GPIO_PIN_MAP(0, 9)    /* P0.09/NFC1, 모듈핀 13 (UICR.NFCPINS off) */

/* UART2 + RS-485 (RAK_DE / RAK_RE#) */
#define PIN_RAK_UART2_RX       NRF_GPIO_PIN_MAP(0, 15)   /* P0.15, 모듈핀 6  */
#define PIN_RAK_UART2_TX       NRF_GPIO_PIN_MAP(0, 16)   /* P0.16, 모듈핀 7  */
#define PIN_RS485_DE           NRF_GPIO_PIN_MAP(0, 17)   /* P0.17, 모듈핀 8  (RAK_DE)  */
#define PIN_RS485_RE_N         NRF_GPIO_PIN_MAP(0, 21)   /* P0.21, 모듈핀 11 (RAK_RE#) */

/* 사용자 입력 / 센서 */
#define PIN_BTN                NRF_GPIO_PIN_MAP(1, 2)    /* P1.02/SW2, 모듈핀 24 */
#define PIN_VIB_SENSE          NRF_GPIO_PIN_MAP(1, 4)    /* P1.04/LED2, 모듈핀 28 */
#define PIN_BAT_AIN            NRF_GPIO_PIN_MAP(0, 5)    /* P0.05/AIN3, 모듈핀 40 */

#define ADC_CH_BAT             NRF_SAADC_INPUT_AIN3

/* QSPI — 외부 Flash U15 (MX25R1635, 16Mbit) */
#define PIN_QSPI_SCK           NRF_GPIO_PIN_MAP(0, 3)    /* P0.03, 모듈핀 29 */
#define PIN_QSPI_CSN           NRF_GPIO_PIN_MAP(0, 26)   /* P0.26, 모듈핀 34 */
#define PIN_QSPI_IO0           NRF_GPIO_PIN_MAP(0, 30)   /* P0.30, 모듈핀 33 */
#define PIN_QSPI_IO1           NRF_GPIO_PIN_MAP(0, 29)   /* P0.29, 모듈핀 32 */
#define PIN_QSPI_IO2           NRF_GPIO_PIN_MAP(0, 28)   /* P0.28, 모듈핀 31 */
#define PIN_QSPI_IO3           NRF_GPIO_PIN_MAP(0, 2)    /* P0.02, 모듈핀 30 */

/* ============================================================
 *  PART 2. MCP23017 I/O Expander (U2)
 *
 *  - 슬레이브 주소 : A2/A1/A0 = GND 가정 → 0x20 (7-bit)
 *  - INTA/INTB    : 회로도상 RESET# 영역과 묶여있음 (필요 시 핀 추가)
 *  - 16개 GPIO를 GPA(8) + GPB(8)로 분할
 * ============================================================ */

#define MCP23017_I2C_ADDR       0x20    /* 7-bit, A0=A1=A2=GND */

/* MCP23017 레지스터 (IOCON.BANK = 0 기본값) */
#define MCP_REG_IODIRA          0x00
#define MCP_REG_IODIRB          0x01
#define MCP_REG_IPOLA           0x02
#define MCP_REG_IPOLB           0x03
#define MCP_REG_GPINTENA        0x04
#define MCP_REG_GPINTENB        0x05
#define MCP_REG_DEFVALA         0x06
#define MCP_REG_DEFVALB         0x07
#define MCP_REG_INTCONA         0x08
#define MCP_REG_INTCONB         0x09
#define MCP_REG_IOCON           0x0A
#define MCP_REG_GPPUA           0x0C
#define MCP_REG_GPPUB           0x0D
#define MCP_REG_INTFA           0x0E
#define MCP_REG_INTFB           0x0F
#define MCP_REG_INTCAPA         0x10
#define MCP_REG_INTCAPB         0x11
#define MCP_REG_GPIOA           0x12
#define MCP_REG_GPIOB           0x13
#define MCP_REG_OLATA           0x14
#define MCP_REG_OLATB           0x15

/* ────────────────────────────────────────────────
 *  MCP23017 핀 비트 정의 — 회로도(tower.png) 매핑
 *
 *  GPA (전원 / LTE / MUX / 카메라 enable 군)
 *    GPA0 → LTE_RST#       (LTE 모뎀 리셋, active low)
 *    GPA1 → (예약 / 미사용)
 *    GPA2 → 12_14V_EN      (12~14V 부스트 enable)
 *    GPA3 → MUX_SEL        (아날로그/시리얼 MUX 선택)
 *    GPA4 → MUX_EN#        (MUX enable, active low)
 *    GPA5 → 3V3_EN         (3.3V 레일 enable)
 *    GPA6 → 5V_EN          (5V 레일 enable)
 *    GPA7 → CAM_EN         (카메라 모듈 전원)
 *
 *  GPB (보조 디바이스 리셋 / 출력 군)
 *    GPB0 → SBC_RST#       (SBC/Luckfox 등 리셋, active low)
 *    GPB1 → BUZZER_EN
 *    GPB2 → (예약)
 *    GPB3 → (예약)
 *    GPB4 → (예약)
 *    GPB5 → (예약)
 *    GPB6 → LED_EN
 *    GPB7 → (예약)
 * ──────────────────────────────────────────────── */

/* GPA 비트 마스크 */
#define MCP_GPA_LTE_RST_N       (1U << 0)
#define MCP_GPA_RSVD1           (1U << 1)
#define MCP_GPA_12_14V_EN       (1U << 2)
#define MCP_GPA_MUX_SEL         (1U << 3)
#define MCP_GPA_MUX_EN_N        (1U << 4)
#define MCP_GPA_3V3_EN          (1U << 5)
#define MCP_GPA_5V_EN           (1U << 6)
#define MCP_GPA_CAM_EN          (1U << 7)

/* GPB 비트 마스크 */
#define MCP_GPB_SBC_RST_N       (1U << 0)
#define MCP_GPB_BUZZER_EN       (1U << 1)
#define MCP_GPB_RSVD2           (1U << 2)
#define MCP_GPB_RSVD3           (1U << 3)
#define MCP_GPB_RSVD4           (1U << 4)
#define MCP_GPB_RSVD5           (1U << 5)
#define MCP_GPB_LED_EN          (1U << 6)
#define MCP_GPB_RSVD7           (1U << 7)

/* 초기화 시 권장 방향(IODIR): 0 = output, 1 = input
 * 모든 enable / reset 라인은 출력. 예약 비트는 안전상 입력(=1)으로 둔다. */
#define MCP_IODIRA_INIT         (MCP_GPA_RSVD1)                              /* GPA1만 입력 */
#define MCP_IODIRB_INIT         (MCP_GPB_RSVD2 | MCP_GPB_RSVD3 |             \
                                 MCP_GPB_RSVD4 | MCP_GPB_RSVD5 |             \
                                 MCP_GPB_RSVD7)                              /* 예약 비트 입력 */

/* 안전한 부팅 기본 출력값(OLAT)
 *   - 모든 *_RST_N 라인은 HIGH (해제)
 *   - 모든 전원 enable는 LOW (꺼짐) — 필요 시 어플리케이션에서 단계적으로 ON
 *   - MUX_EN#는 HIGH (disable) */
#define MCP_OLATA_INIT          (MCP_GPA_LTE_RST_N | MCP_GPA_MUX_EN_N)
#define MCP_OLATB_INIT          (MCP_GPB_SBC_RST_N)

/* ============================================================
 *  PART 3. MCP23017 드라이버 API (구현은 tower.c)
 * ============================================================ */

/**
 * @brief MCP23017 초기화 (I2C, 방향, 초기 출력 설정)
 * @return true 성공
 */
bool tower_mcp_init(void);

/** @brief GPA/GPB 전체 출력 일괄 갱신 */
bool tower_mcp_write_gpa(uint8_t value);
bool tower_mcp_write_gpb(uint8_t value);

/** @brief GPA/GPB 입력 일괄 읽기 */
bool tower_mcp_read_gpa(uint8_t *value);
bool tower_mcp_read_gpb(uint8_t *value);

/** @brief 단일 비트 set / clear / toggle (내부 캐시 사용) */
bool tower_mcp_gpa_set(uint8_t mask);
bool tower_mcp_gpa_clr(uint8_t mask);
bool tower_mcp_gpb_set(uint8_t mask);
bool tower_mcp_gpb_clr(uint8_t mask);

/* ============================================================
 *  PART 4. 보드 레벨 동작 매크로
 *    (active-high enable / active-low reset 의미를 흡수)
 * ============================================================ */

/* 전원 레일 ON/OFF */
#define TOWER_3V3_ON()          tower_mcp_gpa_set(MCP_GPA_3V3_EN)
#define TOWER_3V3_OFF()         tower_mcp_gpa_clr(MCP_GPA_3V3_EN)
#define TOWER_5V_ON()           tower_mcp_gpa_set(MCP_GPA_5V_EN)
#define TOWER_5V_OFF()          tower_mcp_gpa_clr(MCP_GPA_5V_EN)
#define TOWER_12V_ON()          tower_mcp_gpa_set(MCP_GPA_12_14V_EN)
#define TOWER_12V_OFF()         tower_mcp_gpa_clr(MCP_GPA_12_14V_EN)

/* 카메라 전원 */
#define TOWER_CAM_ON()          tower_mcp_gpa_set(MCP_GPA_CAM_EN)
#define TOWER_CAM_OFF()         tower_mcp_gpa_clr(MCP_GPA_CAM_EN)

/* 아날로그 / 시리얼 MUX */
#define TOWER_MUX_ENABLE()      tower_mcp_gpa_clr(MCP_GPA_MUX_EN_N)   /* active low */
#define TOWER_MUX_DISABLE()     tower_mcp_gpa_set(MCP_GPA_MUX_EN_N)
#define TOWER_MUX_SEL_A()       tower_mcp_gpa_clr(MCP_GPA_MUX_SEL)
#define TOWER_MUX_SEL_B()       tower_mcp_gpa_set(MCP_GPA_MUX_SEL)

/* LTE 모뎀 리셋 (active low, 펄스) */
#define TOWER_LTE_RESET_HOLD()  tower_mcp_gpa_clr(MCP_GPA_LTE_RST_N)
#define TOWER_LTE_RESET_REL()   tower_mcp_gpa_set(MCP_GPA_LTE_RST_N)

/* SBC (Luckfox 등) 리셋 */
#define TOWER_SBC_RESET_HOLD()  tower_mcp_gpb_clr(MCP_GPB_SBC_RST_N)
#define TOWER_SBC_RESET_REL()   tower_mcp_gpb_set(MCP_GPB_SBC_RST_N)

/* 부저 / LED */
#define TOWER_BUZZER_ON()       tower_mcp_gpb_set(MCP_GPB_BUZZER_EN)
#define TOWER_BUZZER_OFF()      tower_mcp_gpb_clr(MCP_GPB_BUZZER_EN)
#define TOWER_LED_ON()          tower_mcp_gpb_set(MCP_GPB_LED_EN)
#define TOWER_LED_OFF()         tower_mcp_gpb_clr(MCP_GPB_LED_EN)

/* RS-485 송수신 방향 (MCU 직접 제어) */
#define TOWER_RS485_TX_MODE()   do { nrf_gpio_pin_set(PIN_RS485_DE);   \
                                     nrf_gpio_pin_set(PIN_RS485_RE_N); } while (0)
#define TOWER_RS485_RX_MODE()   do { nrf_gpio_pin_clear(PIN_RS485_DE); \
                                     nrf_gpio_pin_clear(PIN_RS485_RE_N); } while (0)

#ifdef __cplusplus
}
#endif

#endif /* TOWER_H_ */
