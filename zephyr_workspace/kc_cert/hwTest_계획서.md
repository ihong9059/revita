# REVITA_LINK_v1 하드웨어 드라이버 테스트 계획서

KC 인증 기능시험용 펌웨어 개발 전, 각 드라이버를 개별 검증하는 단계별 테스트 계획.

---

## 1. 목적

KC 기능시험 RS485 프로토콜에 정의된 모든 하드웨어 기능을 **개별 드라이버 단위로 먼저 검증**한 후,
RS485 Modbus 통합 펌웨어를 구현한다.

### 개발 흐름

```
Phase 1: 개별 드라이버 HW 테스트 (현재 단계)
    ↓
Phase 2: RS485 Modbus RTU 통신 프레임워크
    ↓
Phase 3: 드라이버 + Modbus 통합 펌웨어
    ↓
Phase 4: PC Host Web UI 연동 검증 (rs485/ 시뮬레이터 활용)
    ↓
Phase 5: KC 인증 시험
```

---

## 2. 공통 사항

| 항목 | 내용 |
|------|------|
| 타겟 보드 | RAK4631 (RAK4630 모듈, nRF52840) |
| 빌드 보드 | `rak4631` |
| 플래시 | PCA10056 J-Link (S/N: 1050295470) 경유 |
| 디버그 콘솔 | uart1 TX=P0.16 → CP210x → `/dev/ttyUSB0`, 115200bps |
| LED 상태 표시 | LED_EN (P0.15) — uart1 RX 대신 GPIO로 사용 |
| 프로젝트 위치 | `zephyr_workspace/kc_cert/hwTest_*` |

---

## 3. Phase 1: 개별 드라이버 HW 테스트

### 3.1 GPIO 출력 — 12V 전원 + 부저 (완료)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_power_buzzer/` |
| 상태 | ✅ 완료 |
| 테스트 핀 | 12V_EN (P0.17), BUZZER_EN (P0.24) |
| 동작 | 키 입력으로 ON/OFF 제어 |
| 결과 | 12V 승압 및 부저 동작 확인 |

### 3.2 GPIO 출력 — X/Y 밸브 모터 (완료)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_power_buzzer/` (모터 추가) |
| 상태 | ✅ 완료 |
| 테스트 핀 | X: EN_A(P0.14), EN_B(P0.13) / Y: EN_A(P0.25), EN_B(P1.01) |
| 동작 | 키 입력으로 STOP/CW/CCW 제어, 12V_EN 자동 HIGH |
| 결과 | X/Y 모터 CW/CCW 방향 전환 확인 |

### 3.3 GPIO 입력 — 버튼 + 진동 센서 (완료)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_digital_input/` |
| 상태 | ✅ 완료 |
| 테스트 핀 | BTN (P0.05, Pull-down), VIB_SENSE (P0.21, Pull-up) |
| 동작 | 50ms 폴링, 상태 변화 시 출력 |
| 결과 | 버튼 PRESS/RELEASE, 진동 감지 확인 |

### 3.4 UART / RS485 통신 (완료)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_rs485/` |
| 상태 | ✅ 완료 |
| 테스트 핀 | TX(P0.20), RX(P0.19), DE(P1.04), RE#(P1.03) |
| 동작 | 1초마다 카운트 송신, 수신 데이터 디버그 출력 |
| 설정 | 9600 bps, 8N1, 반이중 |
| PC 측 | CH340 RS485 동글 → `/dev/ttyUSB1` → CuteCom |

### 3.5 RS485 + LED (완료)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_rs485_led/` |
| 상태 | ✅ 완료 |
| 추가 기능 | LED_EN (P0.15) 1초 점멸, uart1 RX를 P0.02로 이동 |

### 3.6 ADC — 배터리 전압 측정

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_adc/` (예정) |
| 상태 | ⬜ 미착수 |
| 테스트 핀 | BAT_AIN (P0.31 / AIN7) |
| 동작 | 1초마다 ADC 읽기 → mV 환산 (1M/1M 분압 × 2) → 디버그 출력 |
| Zephyr 드라이버 | `adc` |
| 확인 사항 | 전압 범위 2.5V~4.2V, 분압 보정 정확도 |

### 3.7 유량계 펄스 카운팅

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_flowmeter/` (예정) |
| 상태 | ⬜ 미착수 |
| 테스트 핀 | DIO_X (P0.10), DIO_Y (P0.09) |
| 동작 | GPIO 인터럽트로 펄스 카운팅, 1초마다 카운트 출력 |
| 확인 사항 | 12V→3.3V 레벨 변환 후 정확한 카운팅 |
| 외부 장비 | 펄스 발생기 (12V 레벨) 또는 수동 토글 |

### 3.8 Flash — 외부 Flash (MX25R1635F)

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_flash/` (예정) |
| 상태 | ⬜ 미착수 |
| 인터페이스 | QSPI |
| Zephyr 드라이버 | `flash` |
| 동작 | wakeup → erase → write pattern → read back → 검증 → sleep |
| 확인 사항 | QSPI 통신 정상, R/W 데이터 일치 |

### 3.9 LoRa — SX1262 RF 테스트

| 항목 | 내용 |
|------|------|
| 프로젝트 | `lora_tx_test/` (기존), `hwTest_lora/` (예정) |
| 상태 | 🔶 기존 프로젝트 있음, KC 시험 모드 추가 필요 |
| 인터페이스 | SPI (SX1262) |
| Zephyr 드라이버 | `lora` |
| 테스트 항목 | TX carrier, TX modulated, RX continuous, 주파수/파워 변경 |
| 주파수 범위 | 920.9~923.3 MHz |
| 참고 | `RF_Test_채널_파워_설정가이드.md` |

### 3.10 BLE — nRF52840 Radio Test

| 항목 | 내용 |
|------|------|
| 프로젝트 | `ble_radio_test/` (기존), `hwTest_ble/` (예정) |
| 상태 | 🔶 기존 프로젝트 있음, KC 시험 모드 추가 필요 |
| 인터페이스 | nRF52840 내장 Radio |
| 테스트 항목 | Advertising, TX carrier, TX modulated, 채널/파워 설정 |
| 주파수 범위 | 2402~2480 MHz (채널 0~39) |

---

## 4. Phase 2: RS485 Modbus RTU 프레임워크

| 항목 | 내용 |
|------|------|
| 프로젝트 | `hwTest_modbus/` (예정) |
| 목표 | Modbus RTU slave 구현 (Function 0x03, 0x06, 0x10) |
| 기반 | `hwTest_rs485/` 프로젝트에서 확장 |

### 구현 항목

- [ ] Modbus RTU 프레임 파싱 (CRC-16 검증)
- [ ] Function 0x03 (Read Holding Registers) 처리
- [ ] Function 0x06 (Write Single Register) 처리
- [ ] Function 0x10 (Write Multiple Registers) 처리
- [ ] 예외 응답 (0x01, 0x02, 0x03, 0x06)
- [ ] 레지스터 맵 데이터 구조 설계

---

## 5. Phase 3: 드라이버 + Modbus 통합

| 항목 | 내용 |
|------|------|
| 프로젝트 | `kc_cert_firmware/` (예정) |
| 목표 | 전체 드라이버를 Modbus 레지스터로 제어 |

### 통합 순서

| 순서 | 드라이버 | 레지스터 | 비고 |
|------|---------|----------|------|
| 1 | 시험모드 진입/해제 | 0x01FF | BTN + RS485 연동 |
| 2 | 기기 정보 읽기 | 0x0000~0x0004 | 읽기 전용 |
| 3 | 12V 전원 | 0x0122 | GPIO 출력 |
| 4 | 부저 | 0x0120 | GPIO + 타이머 |
| 5 | LED | 0x0121 | GPIO + 점멸 패턴 |
| 6 | 밸브 X/Y | 0x0100~0x0103 | GPIO + duration 타이머 |
| 7 | 배터리 ADC | 0x0010~0x0011 | ADC 읽기 |
| 8 | 진동 센서 | 0x0012~0x0013 | GPIO 입력 + 카운트 |
| 9 | 유량계 | 0x0030~0x0035, 0x0110~0x0113 | 펄스 카운팅 |
| 10 | Flash | 0x0070~0x0071, 0x0150 | QSPI R/W 테스트 |
| 11 | LoRa RF | 0x0050~0x0056, 0x0130~0x013F | carrier/modulated/RX |
| 12 | BLE RF | 0x0060~0x0061, 0x0140~0x014F | carrier/advert/RX |
| 13 | 셀프 테스트 | 0x01F1~0x01F4 | 전체 자동 점검 |
| 14 | 비상 정지 | 0x01F0 | 모든 출력 중지 |

---

## 6. Phase 4: PC Host Web UI 연동 검증

| 항목 | 내용 |
|------|------|
| 기존 시뮬레이터 | `rs485/` 폴더 (Python Flask + DUT simulator) |
| 목표 | DUT simulator 대신 실제 RAK4631 펌웨어와 Web UI 연동 |

### 검증 절차

```
PC (Host Web UI)                    RAK4631 (실제 펌웨어)
┌──────────────┐                    ┌──────────────────┐
│ Flask 웹서버  │                    │ kc_cert_firmware │
│ (host/app.py)│                    │ Modbus Slave     │
└──────┬───────┘                    └────────┬─────────┘
       │ CH340 (USB-RS485)                   │ UART0 (P0.20/P0.19)
       │ /dev/ttyUSB1                        │ DE(P1.04) RE#(P1.03)
       └──── RS485 A/B ─────────────────────┘
```

1. `host/app.py`의 COM 포트를 `/dev/ttyUSB1`로 변경
2. Web UI에서 각 기능 제어 → 실제 하드웨어 동작 확인
3. 시험 시나리오 전체 순서 (프로토콜 문서 8장) 검증

---

## 7. Phase 5: KC 인증 시험

| 항목 | 내용 |
|------|------|
| 시험 장비 | PC + ModbusPoll (또는 Web UI) + USB-RS485 |
| 시험 순서 | KC_기능시험_RS485_프로토콜.md 8장 참조 |
| RF 측정 | 스펙트럼 분석기 |
| 최종 확인 | 셀프 테스트 (RF 포함) 전 항목 PASS |

---

## 8. 현재 진행 상태

| Phase | 항목 | 상태 |
|-------|------|------|
| 1.1 | GPIO 출력 (12V, 부저) | ✅ 완료 |
| 1.2 | GPIO 출력 (X/Y 밸브 모터) | ✅ 완료 |
| 1.3 | GPIO 입력 (BTN, VIB) | ✅ 완료 |
| 1.4 | UART / RS485 통신 | ✅ 완료 |
| 1.5 | RS485 + LED 점멸 | ✅ 완료 |
| 1.6 | ADC (배터리 전압) | ⬜ 다음 |
| 1.7 | 유량계 (펄스 카운팅) | ⬜ 미착수 |
| 1.8 | Flash (QSPI R/W) | ⬜ 미착수 |
| 1.9 | LoRa RF | 🔶 기존 프로젝트 확장 필요 |
| 1.10 | BLE RF | 🔶 기존 프로젝트 확장 필요 |
| 2 | Modbus RTU 프레임워크 | ⬜ 미착수 |
| 3 | 드라이버 + Modbus 통합 | ⬜ 미착수 |
| 4 | Web UI 연동 검증 | ⬜ 미착수 |
| 5 | KC 인증 시험 | ⬜ 미착수 |

---

*기준 문서: KC_기능시험_RS485_프로토콜.md, rs485/시스템_설명서.md*
*작성일: 2026-03-31*
