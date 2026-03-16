# REVITA_LINK HW Test 진행 현황

> 마지막 업데이트: 2026-03-12

---

## 테스트 목록

| # | 테스트명 | 상태 | 결과 | 비고 |
|---|----------|------|------|------|
| 01 | uart2_debug | ✅ 완료 | PASS | UART2 출력 정상 |
| 02 | i2c_scan | ✅ 완료 | PASS | MCP23017 @ 0x20 감지 |
| 03 | mcp23017_basic | ⏳ 대기 | - | GPIO 확장 테스트 |
| 04 | 12v_stepup | ⏳ 대기 | - | 12V 부스트 컨버터 |
| 05 | bat_adc | ⏳ 대기 | - | 배터리 ADC |
| 06 | rs485_loopback | ⏳ 대기 | - | RS485 루프백 |
| 07 | dio_x | ⏳ 대기 | - | DIO X 입력 |
| 08 | dio_y | ⏳ 대기 | - | DIO Y 입력 |
| 09 | button | ⏳ 대기 | - | 외부 버튼 |
| 10 | vib_sensor | ⏳ 대기 | - | 진동 센서 |
| 11 | led | ⏳ 대기 | - | 외부 LED |
| 12 | buzzer | ⏳ 대기 | - | 부저 |
| 13 | motor_x | ⏳ 대기 | - | X축 모터 |
| 14 | motor_y | ⏳ 대기 | - | Y축 모터 |
| 15 | qspi_flash | ✅ 완료 | PASS | JEDEC ID: 0xC2 0x28 0x15 (Macronix MX25R1635F) |

---

## 상태 범례

- ✅ 완료: 테스트 통과
- ❌ 실패: 테스트 실패 (원인 분석 필요)
- 🔧 수정중: 코드 수정 후 테스트 대기
- ⏳ 대기: 아직 시작 안함
- 🔄 진행중: 테스트 진행 중

---

## 완료된 테스트

### test_15_qspi_flash (2026-03-12)

**결과**: PASS

**JEDEC ID**: 0x001528C2
- Manufacturer: 0xC2 (Macronix)
- Memory Type: 0x28
- Capacity: 0x15 (16Mbit)

**Flash 읽기**: 64바이트 순차 데이터 (0x00-0x3F) 정상 읽기

**사용 코드**: Bare-metal C (Arduino 프레임워크 없음)

**핵심 수정사항**:
1. QSPI 핀 매핑 (REVITA_LINK 회로도 기준)
2. **GPIO PIN_CNF 설정** (중요!):
   - TX: `0x00000003` (DIR=Output, INPUT=Disconnect)
   - RX: `0x0000000C` (DIR=Input, PULL=Pullup)

---

## 최근 이슈 및 해결

### 2026-03-12: Bare-metal UART 출력 문제 해결

**문제**: Bare-metal 코드에서 UART 데이터 전송 후 시리얼 출력 안됨

**원인**: GPIO PIN_CNF 레지스터 설정 오류
- 잘못된 설정: TX=0x01, RX=0x00
- 올바른 설정: TX=0x03, RX=0x0C

**검증 방법**:
- `~/revita/hwTest/serial_counter/` Zephyr 프로그램으로 하드웨어 연결 확인
- Zephyr 작동 → Bare-metal GPIO 설정 문제로 확정

**참조**: DEV_GUIDE.md 섹션 3.4

---

## 핀 매핑 정리 (향후작업.txt 기준)

### nRF52840 직접 연결 핀

| 기능 | 핀 |
|------|-----|
| UART1 TX | P0.20 |
| UART1 RX | P0.19 |
| UART2 TX | P0.16 |
| UART2 RX | P0.15 |
| I2C SDA | P0.13 |
| I2C SCL | P0.14 |
| VIB_SENSE | P0.21 |
| DIO_X | P0.10 |
| DIO_Y | P0.09 |
| BAT_AIN | P0.05 (AIN3) |
| RE# | P1.03 |
| DE | P1.04 |
| INTA | P1.01 |
| INTB | P1.02 |

### MCP23017 핀 매핑

**Port A:**
| 핀 | 기능 |
|----|------|
| GPA0 | BTN |
| GPA1 | X_EN_P2 |
| GPA2 | X_EN_B |
| GPA3 | X_EN_A |
| GPA4 | Y_EN_P2 |
| GPA5 | Y_EN_B |
| GPA6 | Y_EN_A |
| GPA7 | 12V_EN |

**Port B:**
| 핀 | 기능 |
|----|------|
| GPB0 | BUZZER_EN |
| GPB7 | LED_EN |

---

*이 파일은 각 테스트 진행 시 업데이트됩니다.*
