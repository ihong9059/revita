# REVITA_LINK_v1 하드웨어 블록 테스트 계획서

## 개요
- **MCU**: RAK4630 (nRF52840 + SX1262 LoRa)
- **GPIO 확장**: MCP23017 (I2C 16-bit I/O)
- **디버깅**: UART2 (TX: P0.16, RX: P0.15) - 모든 테스트에 공통 적용

---

## 테스트 환경

### 타겟 보드 연결
| 타겟 | J-Link S/N | 보드 | 비고 |
|------|------------|------|------|
| Target 1 | 001050234191 | PCA10056 DK | 주 테스트 보드 |
| Target 2 | 001050295470 | PCA10056 DK | 보조 테스트 보드 |

### 디버그 연결
- **Debug Output**: UART2 → USB-UART 어댑터 (CP210x)
- **UART2 TX**: P0.16 → USB-UART RX
- **UART2 RX**: P0.15 → USB-UART TX
- **Baudrate**: 115200
- **Debug Port**: /dev/ttyUSB0

---

## 포트 할당 정리

### nRF52840 직접 연결 핀
| 기능 | 핀 번호 | 비고 |
|------|---------|------|
| UART1_TX | P0.20 | RS485용 |
| UART1_RX | P0.19 | RS485용 |
| UART2_TX | P0.16 | 디버깅용 |
| UART2_RX | P0.15 | 디버깅용 |
| I2C_SDA | P0.13 | MCP23017 연결 |
| I2C_SCL | P0.14 | MCP23017 연결 |
| DE | P1.04 | RS485 송신 enable |
| RE# | P1.03 | RS485 수신 enable |
| DIO_X | P0.10 | 12V 디지털 입력 X |
| DIO_Y | P0.09 | 12V 디지털 입력 Y |
| VIB_SENSE | P0.21 | 진동 센서 |
| BAT_AIN | P0.05 | 배터리 ADC (AIN3) |
| INTA | P1.01 | MCP23017 인터럽트 A |
| INTB | P1.02 | MCP23017 인터럽트 B |
| QSPI_CS | P0.26 | Flash CS |
| QSPI_CLK | P0.03 | Flash CLK |
| QSPI_DIO0 | P0.30 | Flash DIO0 |
| QSPI_DIO1 | P0.29 | Flash DIO1 |
| QSPI_DIO2 | P0.28 | Flash DIO2 |
| QSPI_DIO3 | P0.02 | Flash DIO3 |

### MCP23017 GPIO 확장 (I2C Address: 0x20)

#### GPA (Port A) - 출력
| 기능 | 핀 | 방향 | 비고 |
|------|-----|------|------|
| (미사용) | GPA0 | - | - |
| X_EN_A | GPA1 | OUTPUT | X 모터 A 방향 |
| X_EN_B | GPA2 | OUTPUT | X 모터 B 방향 |
| X_EN_P2 | GPA3 | OUTPUT | X 모터 공통 스위치 (GND) |
| Y_EN_A | GPA4 | OUTPUT | Y 모터 A 방향 |
| Y_EN_B | GPA5 | OUTPUT | Y 모터 B 방향 |
| Y_EN_P2 | GPA6 | OUTPUT | Y 모터 공통 스위치 (GND) |
| **12V_EN** | **GPA7** | **OUTPUT** | **12V 부스트 enable** |

#### GPB (Port B) - 혼합
| 기능 | 핀 | 방향 | 비고 |
|------|-----|------|------|
| **BUZZER_EN** | **GPB0** | **OUTPUT** | **부저 enable** |
| (미사용) | GPB1 | - | - |
| RE# | GPB2 | OUTPUT | RS485 수신 enable (Active Low) |
| RST# | GPB3 | OUTPUT | 외부 리셋 |
| DE | GPB4 | OUTPUT | RS485 송신 enable |
| LED_EN | GPB5 | OUTPUT | LED enable |
| BTN | GPB6 | INPUT | 외부 버튼 |
| VIB_SENSE | GPB7 | INPUT | 진동 센서 |

> **기준 문서**: REVITA_LINK_v1.pdf (Page 12)

---

## 테스트 프로그램 목록 (총 13개)

### Phase 1: 기본 통신 테스트

#### 1. test_01_uart2_debug
- **목적**: UART2 디버깅 출력/입력 확인
- **핀**: TX(P0.16), RX(P0.15)
- **테스트 내용**:
  - "Hello REVITA_LINK!" 출력
  - 입력 에코 테스트
  - 보드레이트: 115200

#### 2. test_02_i2c_scan
- **목적**: I2C 버스 스캔 및 MCP23017 검출
- **핀**: SDA(P0.13), SCL(P0.14)
- **테스트 내용**:
  - I2C 주소 0x00~0x7F 스캔
  - MCP23017 (0x20) 검출 확인

#### 3. test_03_mcp23017_basic
- **목적**: MCP23017 GPIO 확장 IC 기본 동작
- **핀**: I2C + MCP23017 전체
- **테스트 내용**:
  - 레지스터 읽기/쓰기
  - IODIRA/IODIRB 방향 설정
  - GPIOA/GPIOB 출력 테스트

---

### Phase 2: 전원부 테스트

#### 4. test_04_12v_stepup
- **목적**: 12V 부스트 컨버터 동작 확인
- **핀**: 12V_EN (GPA7)
- **테스트 내용**:
  - 12V_EN HIGH → 12V 출력 확인
  - 12V_EN LOW → 출력 OFF 확인
  - 멀티미터로 전압 측정 필요

#### 5. test_05_bat_adc
- **목적**: 배터리 전압 ADC 읽기
- **핀**: BAT_AIN (P0.05/AIN3)
- **테스트 내용**:
  - ADC 초기화
  - 배터리 전압 읽기
  - 전압 변환 (분압비 적용)

---

### Phase 3: 통신 테스트

#### 6. test_06_rs485_loopback
- **목적**: RS485 통신 테스트
- **핀**: UART1_TX(P0.20), UART1_RX(P0.19), DE(P1.04), RE#(P1.03)
- **테스트 내용**:
  - DE/RE# 제어
  - 송신 모드: DE=HIGH, RE#=HIGH
  - 수신 모드: DE=LOW, RE#=LOW
  - 루프백 테스트 (A-B 연결 필요)

---

### Phase 4: 디지털 입력 테스트

#### 7. test_07_dio_x
- **목적**: 12V→3.3V 레벨 변환 입력 X
- **핀**: DIO_X (P0.10)
- **테스트 내용**:
  - 12V 인가 시 LOW 출력 (포토커플러 반전)
  - 입력 상태 모니터링

#### 8. test_08_dio_y
- **목적**: 12V→3.3V 레벨 변환 입력 Y
- **핀**: DIO_Y (P0.09)
- **테스트 내용**:
  - 12V 인가 시 LOW 출력 (포토커플러 반전)
  - 입력 상태 모니터링

#### 9. test_09_button
- **목적**: 외부 버튼 입력 테스트
- **핀**: BTN (GPA0)
- **테스트 내용**:
  - 버튼 누름 감지
  - 인터럽트 기반 처리 (INTA)

#### 10. test_10_vib_sensor
- **목적**: 진동 센서 입력 테스트
- **핀**: VIB_SENSE (P0.21)
- **테스트 내용**:
  - 진동 감지 시 신호 변화 확인
  - 상태 모니터링

---

### Phase 5: 출력 테스트

#### 11. test_11_led
- **목적**: LED 제어 테스트
- **핀**: LED_EN (GPB7)
- **테스트 내용**:
  - LED ON/OFF
  - 점멸 테스트

#### 12. test_12_buzzer
- **목적**: 부저 제어 테스트
- **핀**: BUZZER_EN (GPB0)
- **테스트 내용**:
  - 부저 ON/OFF
  - 비프음 패턴 테스트

---

### Phase 6: 모터 제어 테스트

#### 13. test_13_motor_x
- **목적**: 3선 모터 X 제어 테스트
- **핀**: X_EN_A(GPA3), X_EN_B(GPA2), X_EN_P2(GPA1)
- **테스트 내용**:
  - 정방향: EN_P2=HIGH, EN_A=HIGH, EN_B=LOW
  - 역방향: EN_P2=HIGH, EN_A=LOW, EN_B=HIGH
  - 정지: EN_P2=LOW 또는 EN_A=EN_B=LOW
- **주의**: 12V 부스트가 먼저 활성화되어야 함

#### 14. test_14_motor_y
- **목적**: 3선 모터 Y 제어 테스트
- **핀**: Y_EN_A(GPA6), Y_EN_B(GPA5), Y_EN_P2(GPA4)
- **테스트 내용**:
  - 정방향: EN_P2=HIGH, EN_A=HIGH, EN_B=LOW
  - 역방향: EN_P2=HIGH, EN_A=LOW, EN_B=HIGH
  - 정지: EN_P2=LOW 또는 EN_A=EN_B=LOW
- **주의**: 12V 부스트가 먼저 활성화되어야 함

---

### Phase 7: 메모리 테스트

#### 15. test_15_qspi_flash
- **목적**: QSPI Flash 메모리 테스트
- **IC**: MX25R1635FZUIL0 (16Mbit)
- **핀**: CS(P0.26), CLK(P0.03), DIO0-3
- **테스트 내용**:
  - JEDEC ID 읽기
  - 섹터 지우기
  - 쓰기/읽기 검증

---

## 테스트 순서 (권장)

```
1. test_01_uart2_debug      ← 먼저 디버깅 확인
2. test_02_i2c_scan         ← I2C 버스 확인
3. test_03_mcp23017_basic   ← GPIO 확장 확인
4. test_05_bat_adc          ← 배터리 전압 확인
5. test_04_12v_stepup       ← 12V 전원 확인
6. test_11_led              ← 간단한 출력 확인
7. test_12_buzzer           ← 부저 확인
8. test_09_button           ← 버튼 입력 확인
9. test_07_dio_x            ← 12V 입력 X
10. test_08_dio_y           ← 12V 입력 Y
11. test_10_vib_sensor      ← 진동 센서
12. test_06_rs485_loopback  ← RS485 통신
13. test_13_motor_x         ← 모터 X (12V 필요)
14. test_14_motor_y         ← 모터 Y (12V 필요)
15. test_15_qspi_flash      ← Flash 메모리
```

---

## 개발 환경

- **IDE**: PlatformIO / Arduino IDE / nRF Connect SDK
- **프레임워크**: Arduino (RAK4630 지원)
- **라이브러리**:
  - Wire.h (I2C)
  - Adafruit_MCP23X17.h (MCP23017)
  - SPI.h (QSPI)

---

## 공통 코드 구조

모든 테스트 프로그램에 적용될 공통 구조:

```cpp
// 공통 헤더
#include <Arduino.h>

// UART2 디버깅용 핀 정의
#define DEBUG_TX_PIN 16  // P0.16
#define DEBUG_RX_PIN 15  // P0.15

// 디버깅 출력 함수
void debug_print(const char* msg);
void debug_println(const char* msg);

void setup() {
    // UART2 초기화 (디버깅용)
    Serial1.setPins(DEBUG_RX_PIN, DEBUG_TX_PIN);
    Serial1.begin(115200);

    debug_println("=== Test Program Start ===");

    // 블록별 초기화 코드
    test_init();
}

void loop() {
    // 블록별 테스트 코드
    test_run();

    // 사용자 명령 처리
    handle_command();
}
```

---

## 확인 사항

진행 전 확인이 필요한 사항:

1. **개발 환경**: PlatformIO / Arduino / nRF Connect SDK 중 어떤 것을 사용하시나요?
2. **보드 정의**: RAK4630 보드 정의가 설치되어 있나요?
3. **테스트 장비**: 멀티미터, 오실로스코프, RS485 어댑터 등 준비 상태는?
4. **모터 연결**: 실제 모터가 연결되어 있나요, 아니면 LED로 대체 테스트?

---

*작성일: 2026-03-12*
