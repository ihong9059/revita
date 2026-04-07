/**
 * @file    link.h
 * @brief   REVITA LINK 보드 핀 매핑 헤더 (RAK4630 / nRF52840 기반)
 * @details U1 = RAK4630-9-SM-I 모듈의 22개 좌측 / 22개 우측 핀에서
 *          실제로 사용되는 GPIO만 정의한다. 회로도(link.png) 기준.
 *
 *  RAK4630 모듈 핀 → nRF52840 GPIO → 보드 신호명
 *  ────────────────────────────────────────────
 *   4  P0.13/I2C_SDA      → X_EN_B
 *   5  P0.14/I2C_SCL      → X_EN_A
 *   6  P0.15/UART2_RX     → UART2_RX
 *   7  P0.16/UART2_TX     → UART2_TX
 *   8  P0.17/UART2_DE     → 12V_EN          (R46 10k 직렬)
 *   9  P0.19/UART1_RX     → UART1_RX
 *  10  P0.20/UART1_TX     → UART1_TX
 *  11  P0.21/UART1_DE     → VIB_SENSE
 *  12  P0.10/NFC2         → DIO_X
 *  13  P0.09/NFC1         → DIO_Y
 *  17  NRF_RESET          → RST#            (R3 470k 풀업)
 *  18  SWDCLK             → SWCLK
 *  19  SWDIO              → SWDIO
 *  23  P0.24/I2C_SDA_2    → BUZZER_EN
 *  24  P0.25/I2C_SCL_2    → Y_EN_A
 *  25  P1.01/SW1          → Y_EN_B
 *  26  P1.02/SW2          → Y_EN_P2
 *  27  P1.03/LED1         → RE#  (RS-485 수신 enable, active low)
 *  28  P1.04/LED2         → DE   (RS-485 송신 enable, active high)
 *  29  P0.03/QSPI_CLK     → QSPI_SCLK
 *  30  P0.02/QSPI_DIO3    → QSPI_DIO3
 *  31  P0.28/QSPI_DIO2    → QSPI_DIO2
 *  32  P0.29/QSPI_DIO1    → QSPI_DIO1
 *  33  P0.30/QSPI_DIO0    → QSPI_DIO0
 *  34  P0.26/QSPI_CS      → QSPI_CS
 *  38  P0.31/AIN7         → BAT_AIN         (배터리 전압 측정)
 *  39  P0.05/AIN3         → BTN             (사용자 버튼)
 *  40  P0.04/AIN2         → X_EN_P2
 *
 *  전원: 20/21 VBAT_SX/VBAT_IO_SX = 3.3V (C1,C2 100nF)
 *        43/44 VDD_NRF/VBAT_NRF  = 3.3V (L2 BLM18PG121SN1D + C3,C4 100nF)
 *        14,16,22,35,36,41,42 = GND
 *        15 RF_BT, 37 RF_LORa = 모듈 내부 RF (NC)
 */

#ifndef LINK_H_
#define LINK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  nRF52840 포트/핀 → 절대 핀번호 매크로
 *  (Nordic 표준: P0.x = x, P1.x = 32 + x)
 * ============================================================ */
#ifndef NRF_GPIO_PIN_MAP
#define NRF_GPIO_PIN_MAP(port, pin)   (((port) << 5) | ((pin) & 0x1F))
#endif

/* ============================================================
 *  X 채널 제어 (모터/액추에이터 추정)
 * ============================================================ */
#define PIN_X_EN_A          NRF_GPIO_PIN_MAP(0, 14)   /* P0.14, 모듈핀 5  */
#define PIN_X_EN_B          NRF_GPIO_PIN_MAP(0, 13)   /* P0.13, 모듈핀 4  */
#define PIN_X_EN_P2         NRF_GPIO_PIN_MAP(0, 4)    /* P0.04, 모듈핀 40 */

/* ============================================================
 *  Y 채널 제어
 * ============================================================ */
#define PIN_Y_EN_A          NRF_GPIO_PIN_MAP(0, 25)   /* P0.25, 모듈핀 24 */
#define PIN_Y_EN_B          NRF_GPIO_PIN_MAP(1, 1)    /* P1.01, 모듈핀 25 */
#define PIN_Y_EN_P2         NRF_GPIO_PIN_MAP(1, 2)    /* P1.02, 모듈핀 26 */

/* ============================================================
 *  UART1 (호스트 / 통신용)
 * ============================================================ */
#define PIN_UART1_RX        NRF_GPIO_PIN_MAP(0, 19)   /* P0.19, 모듈핀 9  */
#define PIN_UART1_TX        NRF_GPIO_PIN_MAP(0, 20)   /* P0.20, 모듈핀 10 */

/* ============================================================
 *  UART2 (보조)
 * ============================================================ */
#define PIN_UART2_RX        NRF_GPIO_PIN_MAP(0, 15)   /* P0.15, 모듈핀 6  */
#define PIN_UART2_TX        NRF_GPIO_PIN_MAP(0, 16)   /* P0.16, 모듈핀 7  */

/* ============================================================
 *  RS-485 송수신 방향 제어 (P1.03=LED1, P1.04=LED2)
 *    DE  : HIGH = TX 활성
 *    RE# : LOW  = RX 활성 (active low)
 *  실사용 시 보통 DE와 RE#를 묶어 한 핀으로도 제어 가능.
 * ============================================================ */
#define PIN_RS485_DE        NRF_GPIO_PIN_MAP(1, 4)    /* P1.04, 모듈핀 28 */
#define PIN_RS485_RE_N      NRF_GPIO_PIN_MAP(1, 3)    /* P1.03, 모듈핀 27 */

/* ============================================================
 *  전원 / 액추에이터 enable
 * ============================================================ */
#define PIN_12V_EN          NRF_GPIO_PIN_MAP(0, 17)   /* P0.17, 모듈핀 8  (R46 10k 직렬) */
#define PIN_BUZZER_EN       NRF_GPIO_PIN_MAP(0, 24)   /* P0.24, 모듈핀 23 */

/* ============================================================
 *  센서 / 입력
 * ============================================================ */
#define PIN_VIB_SENSE       NRF_GPIO_PIN_MAP(0, 21)   /* P0.21, 모듈핀 11 (진동 감지) */
#define PIN_BTN             NRF_GPIO_PIN_MAP(0, 5)    /* P0.05 / AIN3, 모듈핀 39 */
#define PIN_BAT_AIN         NRF_GPIO_PIN_MAP(0, 31)   /* P0.31 / AIN7, 모듈핀 38 (배터리) */

/* 아날로그 입력 채널 (SAADC) */
#define ADC_CH_BTN          NRF_SAADC_INPUT_AIN3
#define ADC_CH_BAT          NRF_SAADC_INPUT_AIN7
#define ADC_CH_X_EN_P2      NRF_SAADC_INPUT_AIN2

/* ============================================================
 *  범용 디지털 IO (NFC 핀을 GPIO로 재구성)
 *    주의: P0.09/P0.10을 GPIO로 쓰려면 UICR.NFCPINS 설정 필요
 * ============================================================ */
#define PIN_DIO_X           NRF_GPIO_PIN_MAP(0, 10)   /* P0.10/NFC2, 모듈핀 12 */
#define PIN_DIO_Y           NRF_GPIO_PIN_MAP(0, 9)    /* P0.09/NFC1, 모듈핀 13 */

/* ============================================================
 *  QSPI Flash (모듈 내부 또는 외부 NOR Flash 연결)
 *    nRF52840 QSPI 전용 핀들 - 일반 GPIO로 재할당 금지
 * ============================================================ */
#define PIN_QSPI_SCK        NRF_GPIO_PIN_MAP(0, 3)    /* P0.03, 모듈핀 29 */
#define PIN_QSPI_CSN        NRF_GPIO_PIN_MAP(0, 26)   /* P0.26, 모듈핀 34 */
#define PIN_QSPI_IO0        NRF_GPIO_PIN_MAP(0, 30)   /* P0.30, 모듈핀 33 */
#define PIN_QSPI_IO1        NRF_GPIO_PIN_MAP(0, 29)   /* P0.29, 모듈핀 32 */
#define PIN_QSPI_IO2        NRF_GPIO_PIN_MAP(0, 28)   /* P0.28, 모듈핀 31 */
#define PIN_QSPI_IO3        NRF_GPIO_PIN_MAP(0, 2)    /* P0.02, 모듈핀 30 */

/* ============================================================
 *  SWD 디버그 (모듈핀 18, 19) - 코드에서 직접 제어 안 함
 * ============================================================ */
/* PIN_SWCLK = SWDCLK (모듈핀 18) */
/* PIN_SWDIO = SWDIO  (모듈핀 19) */

/* ============================================================
 *  GPIO 방향/극성 헬퍼
 * ============================================================ */
#define LINK_OUT_HIGH(pin)  nrf_gpio_pin_set(pin)
#define LINK_OUT_LOW(pin)   nrf_gpio_pin_clear(pin)
#define LINK_IN_READ(pin)   nrf_gpio_pin_read(pin)

/* RS-485 방향 매크로 */
#define LINK_RS485_TX_MODE()  do { LINK_OUT_HIGH(PIN_RS485_DE); LINK_OUT_HIGH(PIN_RS485_RE_N); } while (0)
#define LINK_RS485_RX_MODE()  do { LINK_OUT_LOW(PIN_RS485_DE);  LINK_OUT_LOW(PIN_RS485_RE_N);  } while (0)

/* 12V 전원 / 부저 ON-OFF */
#define LINK_12V_ON()         LINK_OUT_HIGH(PIN_12V_EN)
#define LINK_12V_OFF()        LINK_OUT_LOW(PIN_12V_EN)
#define LINK_BUZZER_ON()      LINK_OUT_HIGH(PIN_BUZZER_EN)
#define LINK_BUZZER_OFF()     LINK_OUT_LOW(PIN_BUZZER_EN)

#ifdef __cplusplus
}
#endif

#endif /* LINK_H_ */
