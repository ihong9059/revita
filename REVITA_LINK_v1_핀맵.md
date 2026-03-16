# REVITA_LINK_v1 하드웨어 핀 연결 맵

> **버전**: V1.0
> **작성일**: 2026-03-14
> **목적**: 개발 프로젝트 참조용 핀 맵
> **보드 정의**: `rak4631/nrf52840` (Zephyr RTOS)

---

## 1. nRF52840 (RAK4630) 핀 연결

### 1.1 통신 인터페이스

| 기능 | nRF52840 핀 | 신호명 | 연결 대상 | 비고 |
|------|-------------|--------|-----------|------|
| **UART1 (RS485)** | P0.19 | UART1_RX | MAX3485 RO | RS485 수신 |
| | P0.20 | UART1_TX | MAX3485 DI | RS485 송신 |
| | P0.21 | UART1_DE | MAX3485 DE | 송신 활성화 |
| **UART2 (Debug)** | P0.15 | UART2_RX | DEBUG 커넥터 | 디버그 수신 |
| | P0.16 | UART2_TX | DEBUG 커넥터 | 디버그 송신 |
| | P0.17 | UART2_DE | (미사용) | - |
| **I2C** | P0.13 | I2C_SDA | MCP23017, 센서 | I2C 데이터 |
| | P0.14 | I2C_SCL | MCP23017, 센서 | I2C 클럭 |
| **SWD** | SWCLK | SWCLK | J-Link | 디버그 클럭 |
| | SWDIO | SWDIO | J-Link | 디버그 데이터 |

### 1.2 QSPI Flash (MX25R1635F)

| nRF52840 핀 | 신호명 | 기능 |
|-------------|--------|------|
| P0.26 | QSPI_CS | 칩 셀렉트 |
| P0.03 | QSPI_CLK | 클럭 |
| P0.30 | QSPI_DIO0 | 데이터0 |
| P0.29 | QSPI_DIO1 | 데이터1 |
| P0.28 | QSPI_DIO2 | 데이터2 |
| P0.02 | QSPI_DIO3 | 데이터3 |

### 1.3 디지털 입력 (포토커플러 절연)

| nRF52840 핀 | 신호명 | 연결 대상 | 비고 |
|-------------|--------|-----------|------|
| P0.10 | DIO_X | PC817C | 12V→3.3V 레벨 변환 (반전) |
| P0.09 | DIO_Y | PC817C | 12V→3.3V 레벨 변환 (반전) |

> **참고**: NFC 핀을 GPIO로 사용

### 1.4 MCP23017 인터럽트

| nRF52840 핀 | 신호명 | 연결 대상 | 비고 |
|-------------|--------|-----------|------|
| P1.01 | INTA | MCP23017 INTA | Port A 인터럽트 |
| P1.02 | INTB | MCP23017 INTB | Port B 인터럽트 |

### 1.5 기타

| nRF52840 핀 | 신호명 | 연결 대상 | 비고 |
|-------------|--------|-----------|------|
| P0.31 | BAT_AIN | 배터리 분압 | AIN7 ADC 입력 |
| NRF_RESET | RESET# | TPS3808 | 저전압 리셋 |

---

## 2. SX1262 LoRa (RAK4630 내부 연결)

> **중요**: SX1262는 RAK4630 모듈 내부에서 연결됨. 외부 핀 없음.

| nRF52840 핀 | SX1262 핀 | 기능 |
|-------------|-----------|------|
| P1.10 | NSS | SPI 칩 셀렉트 |
| P1.11 | SCK | SPI 클럭 |
| P1.12 | MOSI | SPI MOSI |
| P1.13 | MISO | SPI MISO |
| P1.06 | NRESET | SX1262 리셋 |
| P1.14 | BUSY | BUSY 신호 |
| P1.15 | DIO1 | 인터럽트 |
| P1.07 | TX_EN | TX 활성화 |
| P1.05 | RX_EN/ANT_SW | RX 활성화/안테나 스위치 |

---

## 3. MCP23017 GPIO 확장 (I2C 주소: 0x20)

### 3.1 Port A (GPA)

| 핀 | 신호명 | 방향 | 연결 대상 | 기능 |
|----|--------|------|-----------|------|
| GPA0 | BTN | IN | EXTERNAL_BTN | 버튼 입력 |
| GPA1 | X_EN_A | OUT | 3-Line Motor X | X축 모터 A방향 |
| GPA2 | X_EN_B | OUT | 3-Line Motor X | X축 모터 B방향 |
| GPA3 | X_EN_P2 | OUT | 3-Line Motor X | X축 모터 GND 스위치 |
| GPA4 | Y_EN_A | OUT | 3-Line Motor Y | Y축 모터 A방향 |
| GPA5 | Y_EN_B | OUT | 3-Line Motor Y | Y축 모터 B방향 |
| GPA6 | Y_EN_P2 | OUT | 3-Line Motor Y | Y축 모터 GND 스위치 |
| GPA7 | 12V_EN | OUT | TPS61178 EN | 12V 부스트 활성화 |

### 3.2 Port B (GPB)

| 핀 | 신호명 | 방향 | 연결 대상 | 기능 |
|----|--------|------|-----------|------|
| GPB0 | BUZZER_EN | OUT | BUZZER1/2 | 부저 활성화 |
| GPB1-6 | (미사용) | - | - | - |
| GPB7 | LED_EN | OUT | EXTERNAL_BTN | LED 활성화 |

---

## 4. 전원 레일

| 레일 | 전압 | 소스 IC | 용도 |
|------|------|---------|------|
| VSOLAR | 5~6V | 입력 | 태양광 패널 입력 |
| VBAT | 3.0~4.2V | CN3158 | 리튬 배터리 |
| LOAD_VCC | 3.0~4.2V | CN3158 | 부하 전원 (=VBAT) |
| 3.3V_VOUT | 3.3V | TPS63001 | MCU, 디지털 IC |
| 12V_SW | 12V | TPS61178+FDS9435A | 모터, RS485, 외부장치 |

---

## 5. 커넥터 핀아웃

### 5.1 RS485_PORT

| 핀 | 신호 | 비고 |
|----|------|------|
| 1 | 12V_SW | 12V 전원 |
| 2 | A | RS485 A (+) |
| 3 | B | RS485 B (-) |
| 4 | GND | 접지 |

### 5.2 4-Line-X / 4-Line-Y (모터 출력)

| 핀 | 신호 | 비고 |
|----|------|------|
| 1 | 12V_SW | 12V 전원 |
| 2 | A | 모터 A 출력 |
| 3 | B | 모터 B 출력 |
| 4 | GND | 접지 |

### 5.3 EXTERNAL_BUTTON

| 핀 | 신호 | 비고 |
|----|------|------|
| 1 | LED_OUT | LED 출력 (3.3V) |
| 2 | BTN_IN | 버튼 입력 (Active Low) |
| 3 | GND | 접지 |

### 5.4 STLINK_PORT (SWD)

| 핀 | 신호 | 비고 |
|----|------|------|
| 1 | 3.3V | 전원 |
| 2 | SWCLK | SWD 클럭 |
| 3 | SWDIO | SWD 데이터 |
| 4 | RESET# | 리셋 |
| 5 | GND | 접지 |

---

## 6. 모터 제어 로직 (3-Line)

```
[X축 모터]
정방향: X_EN_P2=H, X_EN_A=H, X_EN_B=L
역방향: X_EN_P2=H, X_EN_A=L, X_EN_B=H
정지:   X_EN_P2=L

[Y축 모터]
정방향: Y_EN_P2=H, Y_EN_A=H, Y_EN_B=L
역방향: Y_EN_P2=H, Y_EN_A=L, Y_EN_B=H
정지:   Y_EN_P2=L

주의: EN_A와 EN_B를 동시에 HIGH로 설정 금지 (단락 위험)
```

---

## 7. RS485 제어 로직

```
[송신 모드]
- UART1_DE (P0.21) = HIGH

[수신 모드]
- UART1_DE (P0.21) = LOW
```

---

## 8. Zephyr 개발 참고

### 8.1 보드 정의

```bash
# 빌드 명령
west build -b rak4631/nrf52840 <project_path>

# 플래시 명령 (J-Link 사용)
west flash --snr <J-Link_Serial>
```

### 8.2 Devicetree 별칭

| 별칭 | 노드 | 용도 |
|------|------|------|
| lora0 | SX1262 | LoRa 통신 |
| uart0 | UART0 | (nRF 기본) |
| uart1 | UART2 (P0.15/P0.16) | 디버그 콘솔 |
| i2c0 | I2C0 (P0.13/P0.14) | MCP23017, 센서 |
| spi1 | SPI1 (내부) | SX1262 (내부 연결) |

### 8.3 LoRa 설정 (한국 KR920)

```c
#define LORA_FREQUENCY      923000000  // 923 MHz
#define LORA_BANDWIDTH      BW_125_KHZ
#define LORA_SPREADING      SF_7
#define LORA_CODING_RATE    CR_4_5
#define LORA_TX_POWER       14         // dBm
```

---

## 9. J-Link 연결 정보

| 장치 | J-Link Serial | 용도 |
|------|---------------|------|
| Target1 | 001050234191 | 개발보드 1 |
| Target2 | 1050295470 | 개발보드 2 |

---

*이 문서는 REVITA_LINK_v1 하드웨어 개발 시 참조용으로 작성되었습니다.*
