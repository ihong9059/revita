# REVITA_LINK_v1 회로도 블록 목록

> **문서명**: REVITA_LINK_v1
> **버전**: V1.0
> **작성 도구**: EasyEDA
> **총 페이지**: 23 pages

---

## 페이지별 블록 요약

| Page | Block Name | 주요 IC | 기능 |
|------|------------|---------|------|
| 1 | 3.3V_buck_boost | TPS63001DRCR | 3.3V 전압 레귤레이터 |
| 2 | RS485 to UART (3.3V) | MAX3485ESA | RS485-UART 변환 |
| 3 | 12v dio to 3.3v dio | PC817C | 12V→3.3V 레벨 변환 | --> check
| 4 | 12v dio to 3.3v dio_1 | PC817C | 12V→3.3V 레벨 변환 (2번째) | --> check
| 5 | SOLAR_CHARGER_1S | CN3158 | 태양광 배터리 충전기 |
| 6 | 3.0V-watchdog | TPS3808G30DBVR | 전압 감시/리셋 |
| 7 | 12V-0.1A N-CHANNEL_1 | Si2302CDS | N-Channel 스위치 | --> TO_DNG의 의미는?
| 8 | 3-Line-motor-control | FDS9435A, MMBT3904 | 3선 모터 제어 | 
| 9 | 3-Line-motor-control_1 | FDS9435A, MMBT3904 | 3선 모터 제어 (2번째) |
| 10 | 12V-0.1A N-CHANNEL | Si2302CDS | N-Channel 스위치 |
| 11 | 12V-step-up_1 | TPS61178RNWR | 12V 부스트 컨버터 | --> 2 개의 IC용도
| 12 | Schematic1 (main_board) | RAK4630, MCP23017 | 메인 보드 (전체 연결도) |

| 13 | SOLAR_CHARGER_1S | CN3158 | 태양광 배터리 충전기 (복사) |
| 14 | 3.3V_buck_boost | TPS63001DRCR | 3.3V 전압 레귤레이터 (복사) |
| 15 | 3.0V-watchdog | TPS3808G30DBVR | 전압 감시/리셋 (복사) | --> 이유가 있나? BATTERY 
| 16 | RS485 to UART (3.3V) | MAX3485ESA | RS485-UART 변환 (복사) |
| 17 | 3-Line-motor-control | FDS9435A, MMBT3904 | 3선 모터 제어 (복사) |
| 18 | 3-Line-motor-control_1 | FDS9435A, MMBT3904 | 3선 모터 제어_1 (복사) |
| 19 | 12v dio to 3.3v dio | PC817C | 12V→3.3V 레벨 변환 (복사) |
| 20 | 12v dio to 3.3v dio_1 | PC817C | 12V→3.3V 레벨 변환_1 (복사) |
| 21 | 12V-0.1A N-CHANNEL | Si2302CDS | N-Channel 스위치 (복사) |
| 22 | 12V-0.1A N-CHANNEL_1 | Si2302CDS | N-Channel 스위치_1 (복사) |
| 23 | 12V-step-up_1 | TPS61178RNWR | 12V 부스트 컨버터 (복사) |

---

## 상세 블록 설명

### Page 1: 3.3V_buck_boost
- **IC**: TPS63001DRCR (Texas Instruments)
- **기능**: Buck-Boost DC-DC 컨버터
- **입력**: VIN (배터리 전압)
- **출력**: 3.3V_VOUT
- **주요 부품**: L1 2.2uH, C1/C2A/C2B 10uF

---

### Page 2: RS485 to UART (3.3V)
- **IC**: MAX3485ESA (UMW)
- **기능**: RS485 ↔ 3.3V UART 레벨 변환
- **인터페이스**:
  - TX/RX (3.3V UART)
  - A/B (RS485 차동 신호)
  - DE/RE# (송수신 제어)
- **주요 부품**: 바이어스 저항 47K, 종단 저항 10K

---

### Page 3-4: 12v dio to 3.3v dio / 12v dio to 3.3v dio_1
- **IC**: PC817C (포토커플러)
- **기능**: 12V 디지털 입력 → 3.3V 레벨 변환 (절연)
- **입력**: 12V_DIO_IN : DIO_X, DIO_Y(P0.10, P0.09) 
- **출력**: 3.3V_DIO_OUT
- **전류 제한**: R1 1.5K

---

### Page 5, 13: SOLAR_CHARGER_1S
- **IC**: CN3158 (배터리 충전 컨트롤러)
- **기능**: 태양광 패널 → 1S 리튬 배터리 충전
- **보호**: D1 1N5819WS (역전류 방지)
- **온도 감지**: NTC NCP18WF104J03RB
- **충전 전류 설정**: R_ISET 12K (1%)

---

### Page 6, 15: 3.0V-watchdog
- **IC**: TPS3808G30DBVR (Texas Instruments)
- **기능**: 3.0V 저전압 감시 및 리셋 출력
- **입력**: VBAT
- **출력**: RESET# (Active Low)
- **타이밍**: C2 1nF (리셋 지연)

---

### Page 7, 10, 21, 22: 12V-0.1A N-CHANNEL / 12V-0.1A N-CHANNEL_1
- **MOSFET**: Si2302CDS-T1-GE3 (N-Channel)
- **기능**: 3.3V 제어 → 12V/0.1A 로드 스위칭
- **입력**: EN (3.3V Enable)
- **출력**: LOAD_IN → TO_GND
- **풀다운**: RPD 10K

---

### Page 8-9, 17-18: 3-Line-motor-control / 3-Line-motor-control_1
- **P-MOSFET**: FDS9435A (UMW) x2
- **NPN**: MMBT3904 x2
- **N-MOSFET**: Si2302CDS-T1-GE3
- **기능**: 3선 모터 양방향 제어 (A/B 출력)
- **제어 신호**:
  - EN_A: A 방향 활성화
  - EN_B: B 방향 활성화
  - EN_P2: 공통 스위치

Y_EN_A: GPA6 
Y_EN_A: GPA6 
Y_EN_A: GPA6 

Y_EN_A: GPA6 
Y_EN_A: GPA6 
Y_EN_A: GPA6 

- **전원**: 12V_VDD

---

### Page 11, 23: 12V-step-up_1
- **IC**: TPS61178RNWR (Texas Instruments)
- **기능**: 배터리 전압 → 12V 부스트 컨버터
- **입력**: VIN (배터리): GPA7
- **출력**: 12V_VOUT
- **출력 스위치**: FDS9435A (P-MOSFET)
- **주요 설정**:
  - R_FREQ 348K (스위칭 주파수)
  - R_LIMIT 147K (전류 제한)
  - R_UP/R_DOWN 732K/80.6K (출력 전압 설정)

---

### Page 12: Schematic1 (main_board) - WHOLE_PART
- **메인 MCU**: RAK4630-9-SM-I (nRF52840 + SX1262 LoRa)
- **GPIO 확장**: MCP23017-E/SS (I2C 16-bit I/O)
- **플래시 메모리**: MX25R1635FZUIL0 (16Mbit QSPI)

#### 주요 섹션:
| 섹션 | 기능 |
|------|------|
| POWER_PORT | 배터리/태양광 입력 커넥터 |
| POWER_IC | 충전기 + 전압 레귤레이터 |
| MCU_PART | RAK4630 + MCP23017 |
| FLASH_MEMORY | QSPI 플래시 |
| RS485_PORT | RS485 통신 커넥터 |
| STLINK_PORT | SWD 디버그 포트 |
| 4-Line-X/Y | 모터 제어 출력 |
| EXTERNAL_BUTTON | 외부 버튼/LED |
| ANTI_THEIF | 진동 센서 + 부저 |

#### 주요 인터페이스:
- **I2C**: SDA/SCL (MCP23017, 센서용)
- **UART1**: TX/RX/DE/RE# (RS485)
- **UART2**: TX/RX (디버그/확장)
- **QSPI**: CS/CLK/DIO0-3 (플래시)
- **SWD**: SWCLK/SWDIO (디버깅)

#### TVS 보호:
- TVS0500DRVR x3 (입력 보호)
- SM712.TCT (RS485 보호)
- SMAJ16A/SMAJ5.0CA (서지 보호)

---

## 블록 카테고리별 분류

### 전원부 (Power)
| Block | 기능 |
|-------|------|
| SOLAR_CHARGER_1S | 태양광 충전 |
| 3.3V_buck_boost | 3.3V 레귤레이터 |
| 12V-step-up_1 | 12V 부스트 |
| 3.0V-watchdog | 저전압 리셋 |

### 통신부 (Communication)
| Block | 기능 |
|-------|------|
| RS485 to UART (3.3V) | RS485 인터페이스 |
| RAK4630 | LoRa + BLE |

### 입출력부 (I/O)
| Block | 기능 |
|-------|------|
| 12v dio to 3.3v dio | 레벨 변환 (절연) |
| 12V-0.1A N-CHANNEL | 로드 스위치 |
| 3-Line-motor-control | 모터 드라이버 |
| MCP23017 | GPIO 확장 |

---

*작성일: 2026-03-11*
