# REVITA_LINK_v1 하드웨어 제어 보고서

## 1. 시스템 개요

REVITA_LINK_v1은 RAK4630 (nRF52840 기반)을 메인 MCU로 사용하고, MCP23017 I2C GPIO 확장기를 통해 다양한 주변장치를 제어하는 시스템입니다.

---

## 2. 주요 IC 목록

| IC | 기능 | 인터페이스 | 비고 |
|----|------|------------|------|
| RAK4630 (U1) | 메인 MCU (nRF52840) | - | LoRa + BLE |
| MCP23017 (U2) | I2C GPIO 확장기 | I2C (0x20) | 16-bit GPIO |
| MX25R1635F (U15) | QSPI Flash Memory | QSPI | 16Mbit |
| MAX3485ESA | RS485 트랜시버 | UART | Half-duplex |
| TPS61178 (U6) | 12V Step-up | GPIO EN | VBAT → 12V |
| TPS63001 (U4) | 3.3V Buck-Boost | - | VBAT → 3.3V |
| CN3158 (U3) | Solar Charger | - | 1S LiPo |
| TPS3808G30 (U5) | Watchdog/Supervisor | - | 3.0V threshold |

---

## 3. RAK4630 핀 배치

### 3.1 I2C 인터페이스
| 신호 | 핀 | 연결 대상 |
|------|-----|-----------|
| I2C_SDA | P0.13 | MCP23017 |
| I2C_SCL | P0.14 | MCP23017 |
| I2C_SDA_2 | P0.24 | 예비 |
| I2C_SCL_2 | P0.25 | 예비 |

### 3.2 UART 인터페이스
| UART | TX | RX | DE | 용도 |
|------|-----|-----|-----|------|
| UART1 | P0.20 | P0.19 | P0.21 | RS485 |
| UART2 | P0.16 | P0.15 | P0.17 | RS485/Debug |

### 3.3 QSPI Flash 인터페이스
| 신호 | 핀 |
|------|-----|
| QSPI_CS | P0.26 |
| QSPI_CLK | P0.03 |
| QSPI_DIO0 | P0.30 |
| QSPI_DIO1 | P0.29 |
| QSPI_DIO2 | P0.28 |
| QSPI_DIO3 | P0.02 |

### 3.4 ADC 입력
| 신호 | 핀 | 용도 |
|------|-----|------|
| BAT_AIN | P0.31 (AIN7) | 배터리 전압 측정 |
| AIN2 | P0.04 | 예비 |
| AIN3 | P0.05 | 예비 |

### 3.5 기타
| 신호 | 핀 | 용도 |
|------|-----|------|
| SW1 | P1.01 | 스위치 입력 |
| SW2 | P1.02 | 스위치 입력 |
| LED1 | P1.03 | LED 출력 |
| LED2 | P1.04 | LED 출력 |
| NFC1 | P0.09 | NFC |
| NFC2 | P0.10 | NFC |

---

## 4. MCP23017 GPIO 확장기 제어

**I2C 주소: 0x20**

### 4.1 Port A (GPA0-GPA7) - 출력 제어

| 핀 | 신호명 | 기능 | 제어 방식 |
|----|--------|------|-----------|
| GPA0 | BUZZER_EN | 버저 전원 ON/OFF | HIGH=ON |
| GPA1 | 12V_EN | 12V 승압기 Enable | HIGH=ON |
| GPA2 | X_EN_A | X축 모터 A상 | HIGH=ON |
| GPA3 | X_EN_B | X축 모터 B상 | HIGH=ON |
| GPA4 | X_EN_P2 | X축 모터 전원2 | HIGH=ON |
| GPA5 | Y_EN_A | Y축 모터 A상 | HIGH=ON |
| GPA6 | Y_EN_B | Y축 모터 B상 | HIGH=ON |
| GPA7 | Y_EN_P2 | Y축 모터 전원2 | HIGH=ON |

### 4.2 Port B (GPB0-GPB7) - 입/출력 혼합

| 핀 | 신호명 | 방향 | 기능 |
|----|--------|------|------|
| GPB0 | BUZZER_EN | 출력 | 버저 구동 (PWM 가능) |
| GPB1 | LED_EN | 출력 | 외부 LED 제어 |
| GPB2 | BTN | 입력 | 외부 버튼 입력 |
| GPB3 | VIB_SENSE | 입력 | 진동 센서 (SW-18010P) |
| GPB4 | DIO_X | 입력 | 12V DIO X 입력 (포토커플러) |
| GPB5 | DIO_Y | 입력 | 12V DIO Y 입력 (포토커플러) |
| GPB6 | INTA | 출력 | MCP23017 인터럽트 A |
| GPB7 | INTB | 출력 | MCP23017 인터럽트 B |

### 4.3 MCP23017 레지스터 설정 권장값

```c
// Port A: 모두 출력 (0x00)
IODIRA = 0x00;

// Port B: GPB2-GPB5 입력, 나머지 출력
IODIRB = 0x3C;  // 0b00111100

// Port B 풀업 (입력 핀)
GPPUB = 0x3C;   // 0b00111100
```

---

## 5. 전원 시스템

### 5.1 전원 흐름
```
VSOLAR ──┬──> CN3158 ──> VBAT ──┬──> TPS63001 ──> 3.3V
         │                      │
         └──> LOAD_VCC          └──> TPS61178 ──> 12V_SW
                                     (12V_EN으로 제어)
```

### 5.2 12V 전원 제어
| 제어 신호 | 연결 | 동작 |
|-----------|------|------|
| 12V_EN | MCP23017 GPA1 | HIGH=12V 출력 활성화 |

### 5.3 배터리 모니터링
- BAT_AIN (P0.31/AIN7)으로 배터리 전압 측정
- 분압 저항: RDIV1=1M, RDIV2=1M (1/2 분압)
- 실제 전압 = ADC 값 × 2

---

## 6. 모터 제어 (3-Line Motor Control)

### 6.1 X축 모터 (4-Line-X 커넥터)
| 제어 신호 | MCP23017 핀 | 기능 |
|-----------|-------------|------|
| X_EN_A | GPA2 | A상 HIGH-side 스위치 |
| X_EN_B | GPA3 | B상 HIGH-side 스위치 |
| X_EN_P2 | GPA4 | 전원 스위치 (GND 연결) |

### 6.2 Y축 모터 (4-Line-Y 커넥터)
| 제어 신호 | MCP23017 핀 | 기능 |
|-----------|-------------|------|
| Y_EN_A | GPA5 | A상 HIGH-side 스위치 |
| Y_EN_B | GPA6 | B상 HIGH-side 스위치 |
| Y_EN_P2 | GPA7 | 전원 스위치 (GND 연결) |

### 6.3 모터 제어 시퀀스 (예시)
```c
// 모터 정방향
GPA_X_EN_A = HIGH;
GPA_X_EN_B = LOW;
GPA_X_EN_P2 = HIGH;

// 모터 역방향
GPA_X_EN_A = LOW;
GPA_X_EN_B = HIGH;
GPA_X_EN_P2 = HIGH;

// 모터 정지
GPA_X_EN_P2 = LOW;
```

---

## 7. RS485 통신

### 7.1 MAX3485 연결
| 신호 | RAK4630 핀 | 기능 |
|------|------------|------|
| TX | P0.20 (UART1) 또는 P0.16 (UART2) | 송신 데이터 |
| RX | P0.19 (UART1) 또는 P0.15 (UART2) | 수신 데이터 |
| DE | P0.21 (UART1) 또는 P0.17 (UART2) | 송신 활성화 |
| RE# | DE와 연결 | 수신 활성화 (Low) |

### 7.2 RS485 송수신 제어
```c
// 송신 모드
DE = HIGH;  // RE# = HIGH (수신 비활성화)
uart_write(data);
DE = LOW;   // 송신 완료 후 수신 모드로

// 수신 모드
DE = LOW;   // RE# = LOW (수신 활성화)
```

---

## 8. 센서 입력

### 8.1 진동 센서 (SW-18010P)
- 연결: GPB3 (VIB_SENSE)
- 동작: 진동 감지 시 펄스 출력
- 인터럽트 사용 권장

### 8.2 12V DIO 입력 (포토커플러)
| 입력 | MCP23017 핀 | 설명 |
|------|-------------|------|
| 12V_DIO_X | GPB4 (DIO_X) | 12V → 3.3V 레벨 변환 |
| 12V_DIO_Y | GPB5 (DIO_Y) | 12V → 3.3V 레벨 변환 |

- PC817C 포토커플러 사용
- 12V 입력 시 3.3V 출력 LOW (반전)

### 8.3 외부 버튼
- 연결: GPB2 (BTN)
- 4핀 커넥터 (EXTERNAL_BTN): BTN+, LED+, BTN, LED

---

## 9. 출력 장치

### 9.1 버저 (Anti-Theft)
| 구성 요소 | 사양 |
|-----------|------|
| BUZZER1 | 2.7kHz |
| BUZZER2 | 3kHz |
| 제어 | GPB0 (BUZZER_EN) |

### 9.2 LED
- LED_EN (GPB1)으로 외부 LED 제어
- 12V N-Channel MOSFET (Si2302CDS) 사용

---

## 10. Flash Memory (MX25R1635F)

### 10.1 QSPI 인터페이스
| 신호 | RAK4630 핀 |
|------|------------|
| CS# | P0.26 |
| SCLK | P0.03 |
| SI/SIO0 | P0.30 |
| SO/SIO1 | P0.29 |
| WP#/SIO2 | P0.28 |
| HOLD#/SIO3 | P0.02 |

### 10.2 사양
- 용량: 16Mbit (2MB)
- 인터페이스: SPI/Dual SPI/Quad SPI
- 동작 전압: 1.65V ~ 3.6V

---

## 11. 테스트 항목 체크리스트

### 11.1 기본 통신 테스트
- [ ] I2C 통신 (MCP23017 검출)
- [ ] UART2 송수신
- [ ] QSPI Flash 읽기/쓰기

### 11.2 MCP23017 GPIO 테스트
- [x] GPA7 출력 (Y_EN_P2)
- [x] GPB0 출력 (BUZZER_EN)
- [ ] GPA0-GPA6 출력 테스트
- [ ] GPB1 출력 (LED_EN)
- [ ] GPB2 입력 (BTN)
- [ ] GPB3 입력 (VIB_SENSE)
- [ ] GPB4-GPB5 입력 (DIO_X, DIO_Y)

### 11.3 전원 테스트
- [ ] 12V_EN 제어 (GPA1)
- [ ] 배터리 전압 ADC 읽기
- [ ] 3.3V 출력 확인

### 11.4 모터 제어 테스트
- [ ] X축 모터 정/역방향
- [ ] Y축 모터 정/역방향

### 11.5 RS485 테스트
- [ ] UART1 RS485 송수신
- [ ] UART2 RS485 송수신
- [ ] DE 신호 제어

### 11.6 센서 테스트
- [ ] 진동 센서 입력
- [ ] 12V DIO 입력

---

## 12. 핀 매핑 요약표

### RAK4630 직접 제어
| 기능 | 핀 | 방향 |
|------|-----|------|
| I2C_SDA | P0.13 | 양방향 |
| I2C_SCL | P0.14 | 출력 |
| UART1_TX | P0.20 | 출력 |
| UART1_RX | P0.19 | 입력 |
| UART1_DE | P0.21 | 출력 |
| UART2_TX | P0.16 | 출력 |
| UART2_RX | P0.15 | 입력 |
| UART2_DE | P0.17 | 출력 |
| BAT_AIN | P0.31 | ADC 입력 |
| QSPI_* | P0.02,03,26,28-30 | SPI |

### MCP23017 제어 (I2C 0x20)
| 기능 | 포트 | 핀 | 방향 |
|------|------|-----|------|
| BUZZER_EN | GPA | 0 | 출력 |
| 12V_EN | GPA | 1 | 출력 |
| X_EN_A | GPA | 2 | 출력 |
| X_EN_B | GPA | 3 | 출력 |
| X_EN_P2 | GPA | 4 | 출력 |
| Y_EN_A | GPA | 5 | 출력 |
| Y_EN_B | GPA | 6 | 출력 |
| Y_EN_P2 | GPA | 7 | 출력 |
| BUZZER_EN | GPB | 0 | 출력 |
| LED_EN | GPB | 1 | 출력 |
| BTN | GPB | 2 | 입력 |
| VIB_SENSE | GPB | 3 | 입력 |
| DIO_X | GPB | 4 | 입력 |
| DIO_Y | GPB | 5 | 입력 |
| INTA | GPB | 6 | 출력 |
| INTB | GPB | 7 | 출력 |

---

*보고서 작성일: 2026-03-11*
*회로도 버전: REVITA_LINK_v1.0*
