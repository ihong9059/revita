# REVITA_LINK_v1 하드웨어 테스트 - 드라이버/포트 정리

각 driver별 개별 하드웨어 동작 확인을 위한 포트 및 Zephyr 드라이버 정리.

---

## 1. GPIO 출력 (Digital Output)

| 기능 | 핀 | Port | 용도 |
|------|-----|------|------|
| X_EN_A | 5 | P0.14 | X축 모터 A (개방) |
| X_EN_B | 4 | P0.13 | X축 모터 B (폐쇄) |
| X_EN_P2 | 41 | P0.04 | X축 12V MOSFET |
| Y_EN_A | 24 | P0.25 | Y축 모터 A (개방) |
| Y_EN_B | 25 | P1.01 | Y축 모터 B (폐쇄) |
| Y_EN_P2 | 26 | P1.02 | Y축 12V MOSFET |
| BUZZER_EN | 23 | P0.24 | 부저 MOSFET 제어 |
| 12V_EN | 8 | P0.17 | 12V 승압 활성화 |

**Zephyr 드라이버**: `gpio` (`<zephyr/drivers/gpio.h>`)
**테스트 항목**: 각 핀 HIGH/LOW 토글, 밸브 개방/폐쇄, 부저 ON/OFF, 12V ON/OFF

---

## 2. GPIO 입력 (Digital Input)

| 기능 | 핀 | Port | 용도 | 비고 |
|------|-----|------|------|------|
| BTN | 40 | P0.05 | 전원 버튼 | Active Low, Pull-up |
| VIB_SENSE | 11 | P0.21 | 진동 센서 (SW-18010P) | Falling Edge 인터럽트 |

**Zephyr 드라이버**: `gpio` (`<zephyr/drivers/gpio.h>`)
**테스트 항목**: 버튼 입력 감지, 진동 센서 인터럽트 카운팅

---

## 3. GPIO 펄스 카운팅 (유량계)

| 기능 | 핀 | Port | 용도 | 비고 |
|------|-----|------|------|------|
| DIO_X | 12 | P0.10 | X축 유량계 펄스 | 12V→3.3V 레벨 변환 |
| DIO_Y | 13 | P0.09 | Y축 유량계 펄스 | 12V→3.3V 레벨 변환 |

**Zephyr 드라이버**: `gpio` + `counter` (PPI 카운팅) 또는 GPIO 인터럽트
**테스트 항목**: 외부 펄스 인가 후 카운트 정확성 확인

---

## 4. ADC (배터리 전압)

| 기능 | 핀 | Port | 용도 | 비고 |
|------|-----|------|------|------|
| BAT_AIN | 39 | P0.31 (AIN7) | 배터리 전압 측정 | 1M/1M 분압, 인산철 1S |

**Zephyr 드라이버**: `adc` (`<zephyr/drivers/adc.h>`)
**테스트 항목**: ADC 읽기, 전압 환산 (분압 2배), 범위 2.5V~4.2V 확인

---

## 5. UART / RS485

| 기능 | 핀 | Port | 용도 |
|------|-----|------|------|
| RS485 TX | 10 | P0.20 | UART1 TX |
| RS485 RX | 9 | P0.19 | UART1 RX |
| RS485 DE | 28 | P1.04 | 송신 활성화 (Driver Enable) |
| RS485 RE# | 27 | P1.03 | 수신 활성화 (Receiver Enable, Active Low) |

**Zephyr 드라이버**: `uart` (`<zephyr/drivers/uart.h>`) + GPIO (DE/RE# 제어)
**설정**: 9600 bps, 8N1, 반이중
**테스트 항목**: UART 송수신, DE/RE# 방향 전환, Modbus RTU 프레임 에코

---

## 6. LoRa (SX1262 via SPI)

| 기능 | 인터페이스 | 용도 |
|------|-----------|------|
| SX1262 | SPI + GPIO | LoRa 무선 통신 |

**Zephyr 드라이버**: `lora` (`<zephyr/drivers/lora.h>`)
**RAK4631 DTS**: `lora0` alias 사용
**테스트 항목**: LoRa 초기화, TX carrier, TX modulated, RX, RSSI/SNR 읽기
**주파수**: 920.9~923.3 MHz (한국 LoRa 대역)

---

## 7. BLE (nRF52840 내장)

| 기능 | 인터페이스 | 용도 |
|------|-----------|------|
| BLE Radio | nRF52840 내장 | BLE 무선 통신 |

**Zephyr 드라이버**: `bluetooth` (`<zephyr/bluetooth/bluetooth.h>`)
**Radio Test**: `<hal/nrf_radio.h>` (Direct Test Mode)
**테스트 항목**: Advertising, TX carrier, TX modulated, 채널/파워 설정
**주파수**: 2402~2480 MHz

---

## 8. Flash (MX25R1635F via QSPI)

| 기능 | 인터페이스 | 용도 |
|------|-----------|------|
| MX25R1635F | QSPI | 외부 Flash 16Mbit |

**Zephyr 드라이버**: `flash` (`<zephyr/drivers/flash.h>`)
**테스트 항목**: wakeup, erase, write, read back 검증, sleep

---

## 9. 디버그 UART (콘솔 출력)

| 기능 | 핀 | Port | 용도 |
|------|-----|------|------|
| Debug TX | - | P0.16 | 콘솔 출력 (→ CP210x → PC) |
| Debug RX | - | P0.15 | 콘솔 입력 |

**설정**: uart1, 115200 bps, `rak4631.overlay`로 console 지정
**용도**: printk/LOG 출력 확인용 (CuteCom `/dev/ttyUSB0`)

---

## 테스트 순서 (제안)

| 순서 | 드라이버 | 테스트 내용 | 난이도 |
|------|---------|------------|--------|
| 1 | GPIO 출력 | LED, 부저, 12V, 밸브 핀 토글 | 낮음 |
| 2 | GPIO 입력 | 버튼, 진동 센서 읽기 | 낮음 |
| 3 | ADC | 배터리 전압 읽기 | 낮음 |
| 4 | UART/RS485 | 송수신 에코, DE/RE# 제어 | 중간 |
| 5 | 유량계 | 펄스 카운팅 (GPIO 인터럽트) | 중간 |
| 6 | Flash | QSPI R/W 셀프테스트 | 중간 |
| 7 | LoRa | SX1262 초기화, TX/RX | 중간 |
| 8 | BLE | Advertising, Radio Test | 높음 |

---

*기준 문서: KC_기능시험_RS485_프로토콜.md*
*작성일: 2026-03-31*
