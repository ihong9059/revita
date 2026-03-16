# REVITA_LINK 통합 개발 계획서

## 1. 현황 분석

### 1.1 목표
- **최종 목표**: RAK4630 (nRF52840 + SX1262)에서 MeshCore 기반 LoRa Mesh 구현
- **현재 단계**: 하드웨어 블록 테스트 프로그램 작성 및 검증

### 1.2 발견된 문제점

| 항목 | hw_tests (현재) | MeshCore (참조) | 상태 |
|------|----------------|-----------------|------|
| Board Definition | `wiscore_rak4631` (자체 정의) | `rak4631` (MeshCore 정의) | **불일치** |
| Variant | `feather_nrf52840_express` | `WisCore_RAK4631_Board` | **핵심 문제** |
| Framework Version | 미지정 | `1.10700.0` | 불일치 |
| Toolchain | PlatformIO 기본 | `/home/uttec/gnuarmemb` | 불일치 |
| Serial1 Pin Mapping | Feather 기준 | RAK4631 기준 (TX:16, RX:15) | **핵심 문제** |

### 1.3 핵심 문제 원인

**hw_tests의 `feather_nrf52840_express` variant 사용 시 Serial 핀 매핑**:
- Feather variant: `PIN_SERIAL1_TX = 24`, `PIN_SERIAL1_RX = 25`

**MeshCore의 `variants/rak4631/variant.h` 정의**:
- RAK4631 variant: `PIN_SERIAL1_TX = 16`, `PIN_SERIAL1_RX = 15`

**결론**: 현재 hw_tests 코드는 `Serial1.setPins(15, 16)`을 호출하지만, feather variant에서는 핀 매핑이 다르게 적용되어 UART 출력이 동작하지 않음.

---

## 2. 해결 방안

### 2.1 방안 A: MeshCore 구조 재사용 (권장)

hw_tests를 MeshCore 프로젝트 구조 내에서 개발:

```
/home/uttec/revita/MeshCore/
├── boards/rak4631.json          # 보드 정의 (재사용)
├── variants/rak4631/            # RAK4631 variant (재사용)
│   ├── variant.h
│   ├── variant.cpp
│   └── ...
├── examples/
│   └── hw_tests/                # 새로 추가
│       ├── test_01_uart2_debug/
│       ├── test_02_i2c_scan/
│       └── ...
└── platformio.ini               # env 추가
```

**장점**:
- MeshCore와 동일한 환경에서 테스트
- 향후 MeshCore 통합 시 검증된 하드웨어 코드 재사용 가능
- 별도 설정 불필요

### 2.2 방안 B: 독립 프로젝트로 MeshCore 설정 복사

hw_tests를 독립 프로젝트로 유지하되, MeshCore의 설정을 복사:

```
/home/uttec/revita/hw_tests/
├── boards/
│   └── rak4631.json             # MeshCore에서 복사
├── variants/
│   └── rak4631/                 # MeshCore에서 복사
│       ├── variant.h
│       └── variant.cpp
├── common/
│   ├── pin_config.h
│   └── debug_uart.h
└── test_XX_xxx/
    ├── src/main.cpp
    └── platformio.ini
```

---

## 3. 구현 계획 (방안 A 채택)

### Phase 1: 환경 준비

#### 1.1 MeshCore에 hw_tests 환경 추가

**파일**: `/home/uttec/revita/MeshCore/platformio.ini` 수정

```ini
; ----------------- Hardware Tests ---------------------
[hw_test_base]
extends = nrf52_base
board = rak4631
build_flags = ${nrf52_base.build_flags}
  -I variants/rak4631
  -D RAK_4631
  -D HW_TEST_BUILD
  -D PIN_DEBUG_TX=16
  -D PIN_DEBUG_RX=15
debug_tool = jlink
upload_protocol = jlink
upload_flags =
  -SelectEmuBySN
  001050234191
```

#### 1.2 개별 테스트 env 추가

```ini
[env:hw_test_01_uart2]
extends = hw_test_base
build_src_filter = +<../examples/hw_tests/test_01_uart2_debug/>

[env:hw_test_02_i2c_scan]
extends = hw_test_base
lib_deps = ${nrf52_base.lib_deps}
  Wire
build_src_filter = +<../examples/hw_tests/test_02_i2c_scan/>

; ... (각 테스트별 env 추가)
```

### Phase 2: 테스트 코드 이전

#### 2.1 디렉토리 생성
```bash
mkdir -p /home/uttec/revita/MeshCore/examples/hw_tests
```

#### 2.2 테스트 코드 복사 및 수정

**공통 헤더 파일 생성**: `examples/hw_tests/hw_test_common.h`

```cpp
#pragma once

#include <Arduino.h>
#include "variant.h"  // RAK4631 variant 직접 사용

// Debug UART (Serial1 = UART2: TX=P0.16, RX=P0.15)
#define DEBUG_SERIAL Serial1
#define DEBUG_BAUDRATE 115200

inline void debug_init() {
    // RAK4631 variant에서 Serial1은 이미 TX=16, RX=15로 매핑됨
    DEBUG_SERIAL.begin(DEBUG_BAUDRATE);
    while (!DEBUG_SERIAL && millis() < 3000);
}

inline void debug_println(const char* msg) {
    DEBUG_SERIAL.println(msg);
}

inline void debug_print(const char* msg) {
    DEBUG_SERIAL.print(msg);
}

// 테스트 헤더 출력
inline void debug_test_header(const char* test_name) {
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("========================================");
    DEBUG_SERIAL.print("  ");
    DEBUG_SERIAL.println(test_name);
    DEBUG_SERIAL.println("========================================");
    DEBUG_SERIAL.println();
}
```

### Phase 3: 테스트 프로그램 목록

| # | 테스트명 | 목적 | 의존성 |
|---|---------|------|--------|
| 01 | uart2_debug | UART2 디버깅 출력 확인 | 없음 |
| 02 | i2c_scan | I2C 버스 스캔, MCP23017 검출 | Wire |
| 03 | mcp23017_basic | MCP23017 GPIO 기본 동작 | Wire, Adafruit_MCP23X17 |
| 04 | 12v_stepup | 12V 부스트 컨버터 | MCP23017 |
| 05 | bat_adc | 배터리 전압 ADC | 없음 |
| 06 | rs485_loopback | RS485 통신 | Serial2 (TX=20, RX=19) |
| 07 | dio_x | 12V 디지털 입력 X | 없음 |
| 08 | dio_y | 12V 디지털 입력 Y | 없음 |
| 09 | button | 외부 버튼 (MCP23017) | MCP23017 |
| 10 | vib_sensor | 진동 센서 | 없음 |
| 11 | led | LED 제어 (MCP23017) | MCP23017 |
| 12 | buzzer | 부저 제어 (MCP23017) | MCP23017 |
| 13 | motor_x | 3선 모터 X | MCP23017, 12V |
| 14 | motor_y | 3선 모터 Y | MCP23017, 12V |
| 15 | qspi_flash | QSPI Flash | Adafruit_SPIFlash |

### Phase 4: 빌드 및 업로드 절차

#### 4.1 빌드
```bash
cd /home/uttec/revita/MeshCore
pio run -e hw_test_01_uart2
```

#### 4.2 업로드 (J-Link)
```bash
pio run -e hw_test_01_uart2 --target upload
```

#### 4.3 시리얼 모니터
```bash
# UART2 디버그 출력 확인 (USB-UART 어댑터)
pio device monitor -p /dev/ttyUSB0 -b 115200
```

---

## 4. 핀 매핑 참조표

### RAK4631 variant.h 기준

| 기능 | nRF52840 핀 | Arduino 핀 | 비고 |
|------|-------------|------------|------|
| **Serial1 TX (Debug)** | P0.16 | 16 | UART2_TX |
| **Serial1 RX (Debug)** | P0.15 | 15 | UART2_RX |
| **Serial2 TX (RS485)** | P0.20 | 20 | UART1_TX |
| **Serial2 RX (RS485)** | P0.19 | 19 | UART1_RX |
| I2C SDA | P0.13 | 13 | Wire |
| I2C SCL | P0.14 | 14 | Wire |
| LED1 | P1.03 | 35 | Green |
| LED2 | P1.04 | 36 | Blue |
| A0 (BAT) | P0.05 | 5 | AIN3 |

### REVITA_LINK 추가 핀

| 기능 | nRF52840 핀 | 비고 |
|------|-------------|------|
| DE (RS485) | P1.04 | 송신 enable |
| RE# (RS485) | P1.03 | 수신 enable |
| DIO_X | P0.10 | 12V 입력 X |
| DIO_Y | P0.09 | 12V 입력 Y |
| VIB_SENSE | P0.21 | 진동 센서 |
| INTA | P1.01 | MCP23017 인터럽트 |
| INTB | P1.02 | MCP23017 인터럽트 |

---

## 5. J-Link 설정

### 연결된 J-Link 장치
| S/N | 보드 | 용도 |
|-----|------|------|
| 001050234191 | PCA10056 DK | RAK4630 개발 (기본) |
| 001050295470 | PCA10056 DK | 보조 |

### upload_flags 설정
```ini
upload_flags =
  -SelectEmuBySN
  001050234191
```

---

## 6. 작업 순서

### Step 1: MeshCore 프로젝트에 hw_tests 환경 추가
- [ ] platformio.ini에 hw_test_base 및 개별 env 추가
- [ ] examples/hw_tests/ 디렉토리 생성

### Step 2: test_01_uart2_debug 작성 및 테스트
- [ ] hw_test_common.h 작성
- [ ] test_01_uart2_debug/main.cpp 작성
- [ ] 빌드 및 업로드
- [ ] UART2 출력 확인 (/dev/ttyUSB0)

### Step 3: 나머지 테스트 순차 작성
- [ ] test_02_i2c_scan
- [ ] test_03_mcp23017_basic
- [ ] ... (테스트 계획 순서대로)

### Step 4: MeshCore 통합 테스트
- [ ] LoRa 초기화 테스트
- [ ] Mesh 통신 테스트

---

## 7. 예상 이슈 및 해결책

| 이슈 | 원인 | 해결책 |
|------|------|--------|
| Serial 출력 안됨 | Variant 불일치 | MeshCore variant 사용 |
| Toolchain 오류 | 버전 불일치 | gnuarmemb symlink 사용 |
| J-Link 인식 안됨 | 다중 장치 | -SelectEmuBySN 플래그 |
| BLE 크래시 | Bluefruit 버그 | patch_bluefruit.py |

---

## 8. 작업 진행 기록

### 2026-03-12 작업 보고

#### 8.1 완료된 작업

| 항목 | 상태 | 비고 |
|------|------|------|
| Arduino Framework 시도 | ❌ 실패 | Softdevice/Bootloader 없이 HardFault 발생 |
| Zephyr flash_test 검증 | ✅ 성공 | 하드웨어 정상 동작 확인 (카운터 출력) |
| Bare-metal UART 테스트 | ✅ 성공 | Serial 출력 동작 확인 |

#### 8.2 핵심 발견 사항

**Arduino Framework 문제점**:
- Adafruit nRF52 Arduino 프레임워크는 Softdevice S140 + Bootloader 환경을 전제로 동작
- J-Link로 직접 flash 시 초기화 코드 실행 실패로 HardFault 발생
- `Serial1.begin()`이 내부적으로 잘못된 핀으로 초기화되는 문제

**해결 방법 - Bare-metal 접근**:
- Arduino/Softdevice 없이 nRF52840 레지스터 직접 제어
- UARTE0 직접 초기화로 UART 출력 성공

#### 8.3 생성된 파일

```
/home/uttec/revita/MeshCore/examples/hw_tests/test_01_uart2_debug/
├── src/
│   └── main.c           # Bare-metal UART 테스트 코드
├── platformio.ini       # Bare-metal 빌드 설정
└── linker.ld            # 최소 링커 스크립트
```

**platformio.ini 설정**:
```ini
[env:bare_metal_uart]
platform = nordicnrf52
board = nrf52840_dk
build_flags =
    -nostartfiles
    -nostdlib
    -ffreestanding
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=soft
board_build.ldscript = linker.ld
debug_tool = jlink
upload_protocol = jlink
upload_flags =
    -SelectEmuBySN
    001050234191
```

**main.c 핵심 내용**:
- UARTE0 레지스터 직접 접근
- TX: P0.16, RX: P0.15 설정
- Baudrate: 115200 (0x01D7E000)
- 1초 간격 카운터 출력

#### 8.4 테스트 결과

```
========================================
  Bare-metal UART Test
========================================

Hello REVITA_LINK!

0
1
2
...
```

**확인 사항**:
- CuteCom에서 115200 baud로 출력 정상 확인
- 카운터 1초 간격 증가 확인

#### 8.5 다음 단계 옵션

| 옵션 | 설명 | 장단점 |
|------|------|--------|
| A. Bare-metal 계속 | LoRa, I2C 등 bare-metal로 구현 | 완전 제어 가능, 개발 시간 증가 |
| B. Arduino 재시도 | Bootloader 포함 업로드 방식 | MeshCore 호환, 추가 분석 필요 |
| C. Zephyr 이전 | Zephyr RTOS로 전체 이전 | 안정적, MeshCore 재구현 필요 |

#### 8.6 J-Link Flash 명령어

```bash
# Bare-metal UART 테스트 빌드
cd /home/uttec/revita/MeshCore/examples/hw_tests/test_01_uart2_debug
/home/uttec/revita/.venv/bin/platformio run -e bare_metal_uart

# J-Link Flash
JLinkExe -SelectEmuBySN 001050234191 -CommandFile /tmp/jlink_bare_metal.txt
```

**jlink_bare_metal.txt**:
```
device NRF52840_XXAA
si SWD
speed 4000
connect
erase
loadfile /home/uttec/revita/MeshCore/examples/hw_tests/test_01_uart2_debug/.pio/build/bare_metal_uart/firmware.hex
r
g
q
```

---

*작성일: 2026-03-12*
*버전: 1.1*
*최종 수정: 2026-03-12 - Bare-metal UART 테스트 성공 기록 추가*
