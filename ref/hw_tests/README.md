# REVITA_LINK_v1 Hardware Block Tests

RAK4630 기반 하드웨어 블록별 테스트 프로그램

## 디렉토리 구조

```
hw_tests/
├── common/                    # 공통 헤더 파일
│   ├── pin_config.h          # 핀 정의
│   └── debug_uart.h          # UART2 디버깅 함수
│
├── test_01_uart2_debug/      # UART2 디버깅 테스트
├── test_02_i2c_scan/         # I2C 버스 스캔
├── test_03_mcp23017_basic/   # MCP23017 GPIO 확장
├── test_04_12v_stepup/       # 12V 부스트 컨버터
├── test_05_bat_adc/          # 배터리 ADC
├── test_06_rs485_loopback/   # RS485 통신
├── test_07_dio_x/            # 12V DIO X 입력
├── test_08_dio_y/            # 12V DIO Y 입력
├── test_09_button/           # 외부 버튼
├── test_10_vib_sensor/       # 진동 센서
├── test_11_led/              # LED 제어
├── test_12_buzzer/           # 부저 제어
├── test_13_motor_x/          # 모터 X 제어
├── test_14_motor_y/          # 모터 Y 제어
└── test_15_qspi_flash/       # QSPI Flash
```

## 사용법

### 1. PlatformIO 설치

```bash
# VS Code 확장 설치 또는 CLI 설치
pip install platformio
```

### 2. 테스트 빌드 및 업로드

```bash
# 원하는 테스트 디렉토리로 이동
cd hw_tests/test_01_uart2_debug

# 빌드
pio run

# 업로드
pio run -t upload

# 시리얼 모니터
pio device monitor -b 115200
```

### 3. UART2 디버깅 연결

모든 테스트는 UART2를 통해 디버깅 출력을 합니다.

| 신호 | 핀 | 연결 |
|------|-----|------|
| TX | P0.16 | USB-UART RX |
| RX | P0.15 | USB-UART TX |
| GND | GND | USB-UART GND |

보드레이트: **115200**

## 테스트 순서 (권장)

```
1. test_01_uart2_debug      # 먼저 디버깅 확인
2. test_02_i2c_scan         # I2C 버스 확인
3. test_03_mcp23017_basic   # GPIO 확장 확인
4. test_05_bat_adc          # 배터리 전압 확인
5. test_04_12v_stepup       # 12V 전원 확인
6. test_11_led              # LED 출력 확인
7. test_12_buzzer           # 부저 확인
8. test_09_button           # 버튼 입력 확인
9. test_07_dio_x            # 12V 입력 X
10. test_08_dio_y           # 12V 입력 Y
11. test_10_vib_sensor      # 진동 센서
12. test_06_rs485_loopback  # RS485 통신
13. test_13_motor_x         # 모터 X (12V 필요)
14. test_14_motor_y         # 모터 Y (12V 필요)
15. test_15_qspi_flash      # Flash 메모리
```

## 핀 맵 요약

### nRF52840 직접 핀

| 기능 | 핀 |
|------|-----|
| UART2_TX (Debug) | P0.16 |
| UART2_RX (Debug) | P0.15 |
| UART1_TX (RS485) | P0.20 |
| UART1_RX (RS485) | P0.19 |
| I2C_SDA | P0.13 |
| I2C_SCL | P0.14 |
| RS485_DE | P1.04 |
| RS485_RE# | P1.03 |
| DIO_X | P0.10 |
| DIO_Y | P0.09 |
| VIB_SENSE | P0.21 |
| BAT_AIN | P0.05 |

### MCP23017 GPIO (I2C 0x20)

| 기능 | 핀 |
|------|-----|
| BTN | GPA0 (Input) |
| X_EN_P2 | GPA1 |
| X_EN_B | GPA2 |
| X_EN_A | GPA3 |
| Y_EN_P2 | GPA4 |
| Y_EN_B | GPA5 |
| Y_EN_A | GPA6 |
| 12V_EN | GPA7 |
| BUZZER_EN | GPB0 |
| LED_EN | GPB7 |

## 주의사항

1. **12V 테스트 전 주의**: 모터, 부저 테스트 시 12V 부스트가 먼저 활성화되어야 함
2. **RS485 테스트**: 루프백 테스트 시 A-B 단자를 연결하거나 RS485 동글 필요
3. **Flash 테스트**: 섹터 지우기는 기존 데이터를 삭제함

## 문제 해결

### UART2 출력이 안 보일 때
- TX/RX 선 연결 확인 (크로스 연결)
- 보드레이트 115200 확인
- 올바른 COM 포트 선택

### MCP23017 검출 안 됨
- I2C 핀 연결 확인
- MCP23017 전원 확인 (3.3V)
- I2C 주소 0x20 확인

### Flash 인식 안 됨
- QSPI 핀 연결 확인
- Flash 전원 확인

---

*작성일: 2026-03-12*
