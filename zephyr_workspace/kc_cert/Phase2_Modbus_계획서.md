# Phase 2: RS485 Modbus RTU 펌웨어 개발 계획서

---

## 1. 목표

RAK4631(DUT)에 Modbus RTU Slave를 구현하고, PC Host Web UI(Flask)에서
RS485 동글(CH340)을 통해 실제 하드웨어를 제어/모니터링하는 기본 통신을 검증한다.

---

## 2. 시스템 구성

```
PC (Raspberry Pi)                         RAK4631 (DUT)
┌────────────────────┐                    ┌────────────────────┐
│  Flask Web UI      │                    │  Modbus RTU Slave  │
│  (host/app.py)     │                    │  (Zephyr 펌웨어)    │
│  localhost:5000     │                    │                    │
└────────┬───────────┘                    │  RS485 UART0       │
         │                                │  TX=P0.20          │
    /dev/ttyUSB1                          │  RX=P0.19          │
    CH340 (RS485 동글)                     │  DE=P1.04          │
         │                                │  RE#=P1.03         │
         └──── RS485 A/B ────────────────┘                    │
                                          │  Debug UART1       │
                                          │  TX=P0.16          │
                                          │  → CP210x          │
                                          │  → /dev/ttyUSB0    │
                                          │  → CuteCom 115200  │
                                          └────────────────────┘
```

| 구성 | 장치 | 포트 | 용도 |
|------|------|------|------|
| Host Web UI | PC Flask 서버 | CH340 `/dev/ttyUSB1` | Modbus Master (명령 송신) |
| DUT 펌웨어 | RAK4631 uart0 | P0.20/P0.19, 9600bps | Modbus Slave (명령 수신/응답) |
| 디버그 콘솔 | RAK4631 uart1 | CP210x `/dev/ttyUSB0`, 115200bps | 로그 출력 |
| LED 상태 | P0.15 | GPIO | 동작 상태 표시 |

---

## 3. 개발 단계

### Step 1: Modbus RTU 프레임 처리 기본

**프로젝트**: `kc_cert/kc_firmware/`

| 항목 | 내용 |
|------|------|
| 목표 | Modbus RTU 프레임 수신, CRC 검증, Function Code 분기, 응답 전송 |
| 통신 | UART0 (9600, 8N1) + DE/RE# 반이중 제어 |
| 디버그 | UART1에 수신/송신 프레임 hex dump 출력 |

**구현 모듈:**

```
kc_firmware/src/
├── main.c           # 메인 루프, 초기화
├── modbus.c/.h      # Modbus RTU 프레임 수신/파싱/응답/CRC
├── rs485.c/.h       # UART0 송수신, DE/RE# 제어
└── registers.c/.h   # 레지스터 맵 데이터 구조
```

**Modbus 기능:**

| Function | 용도 | 구현 내용 |
|----------|------|----------|
| 0x03 | Read Holding Registers | 레지스터 맵에서 값 읽기 |
| 0x06 | Write Single Register | 단일 레지스터 쓰기 |
| 0x10 | Write Multiple Registers | 다중 레지스터 쓰기 |
| 예외 | Error Response | 0x01(Illegal Function), 0x02(Illegal Address), 0x03(Illegal Value) |

**CRC-16 계산:**
- 다항식: 0x8005, 초기값: 0xFFFF (Modbus 표준)
- DUT simulator의 `modbus_crc.py`와 동일한 알고리즘

**프레임 수신 방식:**
- UART 인터럽트로 바이트 수집
- 3.5 character time silence 감지로 프레임 종료 판단 (9600bps → ~4ms)
- 또는 타이머 기반 프레임 완료 감지

### Step 2: 레지스터 맵 + 기본 드라이버 연동

**시험모드 진입/해제부터 시작하여 검증된 드라이버를 순차 연동.**

| 순서 | 레지스터 | 기능 | 드라이버 | Phase 1 검증 |
|------|----------|------|---------|-------------|
| 1 | 0x01FF | 시험모드 진입/해제 | - | - |
| 2 | 0x0000~0x0004 | 기기 정보 읽기 (ID, 버전) | 읽기 전용 | - |
| 3 | 0x0122 | 12V 전원 ON/OFF | GPIO (P0.17) | ✅ 완료 |
| 4 | 0x0120 | 부저 ON/OFF/타이머 | GPIO (P0.24) + 타이머 | ✅ 완료 |
| 5 | 0x0121 | LED 제어 (ON/OFF/점멸) | GPIO (P0.15) | ✅ 완료 |
| 6 | 0x0100~0x0103 | 밸브 X/Y 제어 | GPIO + duration 타이머 | ✅ 완료 |
| 7 | 0x0010~0x0011 | 배터리 전압 읽기 | ADC (AIN7) | ✅ 완료 |
| 8 | 0x0012~0x0014 | 진동/버튼 상태 | GPIO 입력 | ✅ 완료 |
| 9 | 0x0070~0x0071, 0x0150 | Flash 셀프 테스트 | QSPI Flash | ✅ 완료 |
| 10 | 0x01F0 | 비상 정지 (ALL STOP) | 전체 GPIO OFF | - |

### Step 3: Host Web UI 연동 검증

| 항목 | 내용 |
|------|------|
| Host | 기존 `rs485/host/app.py` 재활용 |
| 변경 | COM 포트를 `/dev/ttyUSB1`로 변경 |
| 검증 | Web UI에서 각 기능 제어 → 실제 하드웨어 동작 확인 |

**검증 시나리오:**

```
1. 브라우저 접속 → localhost:5000
2. [시험모드 진입] → DUT 응답 에코 확인
3. [기기 정보 읽기] → ID, 버전 표시
4. [12V ON/OFF] → 실제 12V 승압 동작
5. [부저 3초] → 실제 부저 울림
6. [LED ON/점멸] → 실제 LED 동작
7. [밸브 X 개방 5초] → 모터 동작, 자동 정지
8. [배터리 전압 읽기] → 실제 ADC 값 표시
9. [ALL STOP] → 전체 출력 즉시 정지
```

---

## 4. 레지스터 맵 데이터 구조 (C)

```c
/* 레지스터 맵 - 읽기 영역 */
struct reg_read {
    /* 0x0000~0x0004: 기기 정보 */
    uint16_t device_id_h;     /* 0x0000 */
    uint16_t device_id_l;     /* 0x0001 */
    uint16_t fw_version;      /* 0x0002 */
    uint16_t hw_version;      /* 0x0003 */
    uint16_t test_mode;       /* 0x0004 */

    /* 0x0010~0x0015: 전원/센서 */
    uint16_t bat_voltage;     /* 0x0010 mV */
    uint16_t bat_percent;     /* 0x0011 % */
    uint16_t vib_status;      /* 0x0012 */
    uint16_t vib_count;       /* 0x0013 */
    uint16_t btn_status;      /* 0x0014 */
    uint16_t supply_12v;      /* 0x0015 */

    /* 0x0020~0x0023: 밸브 상태 */
    uint16_t valve_x_state;   /* 0x0020 */
    uint16_t valve_y_state;   /* 0x0021 */

    /* 0x0040~0x0041: 출력 장치 */
    uint16_t buzzer_state;    /* 0x0040 */
    uint16_t led_state;       /* 0x0041 */

    /* 0x0070~0x0071: Flash */
    uint16_t flash_state;     /* 0x0070 */
    uint16_t flash_test;      /* 0x0071 */
};
```

---

## 5. Modbus 프레임 처리 흐름

```
UART0 RX 인터럽트
    ↓
바이트 버퍼에 저장
    ↓
3.5 char silence 감지 (타이머)
    ↓
프레임 완료
    ↓
Slave Address 확인 (0x01)
    ↓
CRC-16 검증
    ↓
Function Code 분기
    ├── 0x03 → read_holding_registers()
    ├── 0x06 → write_single_register()
    ├── 0x10 → write_multiple_registers()
    └── 기타 → exception_response(0x01)
    ↓
응답 프레임 생성 + CRC 부착
    ↓
DE=HIGH → UART0 TX 송신 → DE=LOW
    ↓
디버그 콘솔(UART1)에 로그 출력
```

---

## 6. 파일 구조

```
kc_cert/
├── kc_firmware/                  ← Phase 2 메인 프로젝트
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── rak4631.overlay
│   └── src/
│       ├── main.c               # 초기화, 메인 루프
│       ├── modbus.c/.h          # Modbus RTU 프레임 처리, CRC
│       ├── rs485.c/.h           # UART0 송수신, DE/RE# 제어
│       ├── registers.c/.h       # 레지스터 맵, 읽기/쓰기 핸들러
│       └── drivers.c/.h         # GPIO/ADC/Flash 드라이버 래퍼
│
├── rs485/                        ← 기존 시뮬레이터 (Host 재활용)
│   ├── host/
│   │   ├── app.py               # Flask Web UI (Master)
│   │   └── templates/index.html
│   └── dut/
│       └── dut_simulator.py     # Python 시뮬레이터 (더 이상 불필요)
│
└── hwTest_*/                     ← Phase 1 개별 드라이버 테스트
```

---

## 7. 구현 우선순위

| 우선순위 | 항목 | 설명 |
|---------|------|------|
| **P0** | RS485 통신 + Modbus CRC | 프레임 수신/송신/검증 기본 동작 |
| **P0** | Function 0x03 (Read) | 기기 정보 읽기로 통신 검증 |
| **P0** | Function 0x06 (Write) | 12V ON/OFF로 제어 검증 |
| **P1** | Function 0x10 (Write Multiple) | 밸브 제어 (CMD + duration) |
| **P1** | 시험모드 진입/해제 | 0x01FF 레지스터 |
| **P1** | 타이머 기반 자동 정지 | 부저, 밸브 duration 후 자동 OFF |
| **P2** | 예외 응답 | Illegal Function/Address/Value |
| **P2** | 비상 정지 | ALL STOP (0x01F0) |
| **P3** | Host Web UI 연동 | 기존 Flask app.py 포트 변경 후 검증 |

---

## 8. Host Web UI 실행 방법 (Linux)

```bash
# 패키지 설치
pip3 install pyserial flask

# Host 실행 (CH340 RS485 동글)
cd /home/uttec/revita/zephyr_workspace/kc_cert/rs485
python3 host/app.py /dev/ttyUSB1 9600 5000

# 브라우저 접속
# http://localhost:5000
```

---

## 9. 검증 기준

| 항목 | 기준 |
|------|------|
| 통신 | Host에서 Modbus 프레임 송신 → DUT 정상 응답 (CRC 일치) |
| 읽기 | 0x03으로 기기 정보, 배터리 전압, 상태값 정상 읽기 |
| 쓰기 | 0x06으로 12V, 부저, LED 제어 → 실제 하드웨어 동작 |
| 다중 쓰기 | 0x10으로 밸브 CMD + duration → 모터 동작 + 자동 정지 |
| 예외 | 잘못된 주소/값 → 예외 응답 (Function + 0x80) |
| Web UI | 브라우저에서 버튼 클릭 → 실제 동작 → 상태 표시 갱신 |
| 디버그 | UART1에 모든 TX/RX 프레임 hex dump 출력 |

---

*기준 문서: KC_기능시험_RS485_프로토콜.md, rs485/시스템_설명서.md*
*작성일: 2026-03-31*
