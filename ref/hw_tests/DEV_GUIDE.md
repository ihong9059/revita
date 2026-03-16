# REVITA_LINK HW Test 개발 가이드

> **중요**: 이 문서는 반복되는 실수를 방지하기 위한 가이드입니다.
> 새 세션 시작 시 반드시 확인하세요.

---

## 1. 환경 설정

### 1.1 PlatformIO 경로
```bash
# PlatformIO 실행
~/revita/hw_tests/.venv/bin/platformio run

# 또는 축약형
PIO=~/revita/hw_tests/.venv/bin/platformio
$PIO run
```

### 1.2 J-Link Serial Number
```
J-Link S/N: 001050234191
```

---

## 2. 빌드 및 업로드

### 2.1 표준 빌드 명령
```bash
cd ~/revita/hw_tests/test_XX_name
~/revita/hw_tests/.venv/bin/platformio run
```

### 2.2 업로드 방법 (중요!)

#### 방법 A: J-Link 사용 (권장)

**platformio.ini에 추가:**
```ini
upload_protocol = jlink
debug_tool = jlink
upload_flags =
    -SelectEmuBySN
    001050234191
```

**업로드 명령:**
```bash
~/revita/hw_tests/.venv/bin/platformio run -t upload
```

#### 방법 B: 수동 J-Link Flash

시리얼 포트 충돌이나 nrfutil 문제 시 사용:

```bash
# 1. JLink 명령 파일 생성
cat > /tmp/jlink_flash.txt << 'EOF'
device NRF52840_XXAA
si SWD
speed 4000
connect
erase
loadfile /home/uttec/revita/hw_tests/test_XX_name/.pio/build/rak4631/firmware.hex
r
g
q
EOF

# 2. J-Link 실행
JLinkExe -SelectEmuBySN 001050234191 -CommandFile /tmp/jlink_flash.txt
```

### 2.3 시리얼 포트 충돌 해결

업로드 전 시리얼 모니터 프로그램 종료 필요:
```bash
# 포트 사용 프로세스 확인
lsof /dev/ttyUSB0

# cutecom 등 종료
kill $(lsof -t /dev/ttyUSB0)
```

---

## 3. 알려진 문제 및 해결책

### 3.1 QSPI Flash 핀 매핑 문제

**문제**: JEDEC ID가 0x00으로 표시됨

**원인**: `feather_nrf52840_express` variant의 QSPI 핀이 REVITA_LINK와 다름

**해결**: 커스텀 핀 지정 필요

```cpp
// REVITA_LINK QSPI 핀 매핑
#define REVITA_QSPI_SCK   19   // -> P0.03
#define REVITA_QSPI_CS    9    // -> P0.26
#define REVITA_QSPI_IO0   16   // -> P0.30
#define REVITA_QSPI_IO1   20   // -> P0.29
#define REVITA_QSPI_IO2   17   // -> P0.28
#define REVITA_QSPI_IO3   18   // -> P0.02

Adafruit_FlashTransport_QSPI flashTransport(
    REVITA_QSPI_SCK, REVITA_QSPI_CS,
    REVITA_QSPI_IO0, REVITA_QSPI_IO1,
    REVITA_QSPI_IO2, REVITA_QSPI_IO3
);
```

**핀 매핑 테이블** (feather_nrf52840_express variant 기준):

| nRF52840 GPIO | Arduino 핀 | 용도 |
|---------------|------------|------|
| P0.02 | 18 | QSPI IO3 |
| P0.03 | 19 | QSPI SCK |
| P0.26 | 9  | QSPI CS |
| P0.28 | 17 | QSPI IO2 |
| P0.29 | 20 | QSPI IO1 |
| P0.30 | 16 | QSPI IO0 |

### 3.2 nrfutil 업로드 실패

**문제**: `--singlebank` 옵션 오류

**해결**: J-Link 업로드 사용 (섹션 2.2 참조)

### 3.3 Arduino Framework HardFault

**문제**: Softdevice 없이 Arduino 코드 실행 시 HardFault

**해결**:
- J-Link 업로드 시 `upload_protocol = jlink` 사용
- 또는 Bare-metal 코드 사용 (test_01 참조)

### 3.4 Bare-metal UART 출력 안됨 (중요!)

**문제**: Bare-metal 코드에서 UART 초기화 후 출력이 안됨

**원인**: GPIO 핀 설정이 잘못됨

**해결**: GPIO PIN_CNF 레지스터를 올바르게 설정:
```c
// 잘못된 설정 (출력 안됨)
P0_PIN_CNF(TX_PIN) = 0x00000001;  // DIR=Output만 설정
P0_PIN_CNF(RX_PIN) = 0x00000000;  // 기본값

// 올바른 설정 (작동함)
P0_PIN_CNF(TX_PIN) = 0x00000003;  // DIR=Output, INPUT=Disconnect
P0_PIN_CNF(RX_PIN) = 0x0000000C;  // DIR=Input, PULL=Pullup
```

**검증 방법**: `~/revita/hwTest/serial_counter/` Zephyr 프로그램으로 하드웨어 확인
```bash
JLinkExe -SelectEmuBySN 001050234191 -CommandFile /tmp/jlink_flash.txt
# firmware: ~/revita/hwTest/serial_counter/build/serial_counter/zephyr/zephyr.hex
```

### 3.5 Bare-metal platformio.ini 템플릿

```ini
[env:bare_metal]
platform = nordicnrf52
board = nrf52840_dk
; NO framework - bare metal
build_flags =
    -nostartfiles
    -nostdlib
    -ffreestanding
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=soft
build_unflags =
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
board_build.ldscript = linker.ld
debug_tool = jlink
upload_protocol = jlink
upload_flags =
    -SelectEmuBySN
    001050234191
```

---

## 4. 테스트 프로그램 공통 구조

### 4.1 디렉토리 구조
```
test_XX_name/
├── src/
│   └── main.cpp
├── platformio.ini
└── .pio/          (빌드 결과물)
```

### 4.2 공통 헤더 위치
```
~/revita/hw_tests/common/
├── pin_config.h   # REVITA_LINK 핀 정의
└── debug_uart.h   # UART2 디버그 출력
```

### 4.3 platformio.ini 템플릿
```ini
[platformio]
boards_dir = ../boards

[env:rak4631]
platform = nordicnrf52
board = wiscore_rak4631
framework = arduino
monitor_speed = 115200

; J-Link 업로드 설정
upload_protocol = jlink
debug_tool = jlink
upload_flags =
    -SelectEmuBySN
    001050234191

build_flags =
    -DSERIAL_BUFFER_SIZE=256
    -DNRF52840_XXAA
    -I../common

lib_deps =
    adafruit/Adafruit TinyUSB Library
    ; 필요한 라이브러리 추가
```

---

## 5. 시리얼 모니터

### 5.1 설정
- 포트: `/dev/ttyUSB0`
- Baud rate: `115200`
- 프로그램: CuteCom 또는 `screen /dev/ttyUSB0 115200`

### 5.2 테스트 명령어
각 테스트 프로그램은 단일 문자 명령 사용:
- `h` - 도움말
- `t` - 전체 테스트
- 기타 테스트별 명령

---

## 6. 빠른 참조

### 빌드 & 업로드 (J-Link)
```bash
cd ~/revita/hw_tests/test_XX_name
~/revita/hw_tests/.venv/bin/platformio run -t upload
```

### 시리얼 모니터
```bash
screen /dev/ttyUSB0 115200
# 종료: Ctrl+A, K, y
```

### 포트 충돌 해결
```bash
kill $(lsof -t /dev/ttyUSB0)
```

---

*최종 수정: 2026-03-12*
