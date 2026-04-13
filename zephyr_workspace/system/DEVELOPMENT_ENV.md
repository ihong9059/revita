# REVITA 시스템 — 개발환경 및 RTOS 분석

> **작성일**: 2026-04-13
> **대상**: Tower (RAK4631) / Link-Node (RAK4631)
> **기준**: `system/driver/` 드라이버 테스트 결과 + `FEATURE_LIST.md` 요구사항

---

## 1. 현재 개발환경

| 항목 | 내용 |
|------|------|
| MCU | nRF52840 (RAK4631 모듈) — Tower / Link 동일 |
| RTOS | Zephyr RTOS (NCS v2.x 기반) |
| 빌드 시스템 | west + CMake (`west build -b rak4631/nrf52840`) |
| 디버그 | J-Link SWD (Tower S/N 1050295470 / Link S/N 1050234191) |
| Host | Raspberry Pi (Linux), Flask 웹 서버 |
| 언어 | C (펌웨어), Python (Host 웹) |

### 빌드 환경 설정

```bash
source /home/uttec/ncs/.venv/bin/activate
export ZEPHYR_BASE=/home/uttec/ncs/zephyr

# 빌드
cd /home/uttec/revita/zephyr_workspace/system/driver/<프로젝트>/<타겟>
west build -b rak4631/nrf52840 --pristine -d build

# 플래시
west flash --runner jlink --dev-id 1050295470 -d build  # Tower
west flash --runner jlink --dev-id 1050234191 -d build  # Link
```

---

## 2. 현재까지 검증된 Zephyr 기능

드라이버 테스트(`system/driver/`)를 통해 실제 HW에서 검증 완료된 Zephyr 기능 목록.

| Zephyr 기능 | API / Kconfig | 사용처 | 검증 상태 |
|-------------|---------------|--------|-----------|
| 커널 타이머 | `k_msleep()`, `k_uptime_get()` | 전체 드라이버 폴링 루프 | ✅ |
| 커널 busy wait | `k_busy_wait()` | RS485 TX→RX 전환 2ms flush | ✅ |
| UART IRQ | `uart_irq_callback_set()` | USB CDC RX, RS485 RX | ✅ |
| GPIO 드라이버 | `gpio_pin_configure()`, `gpio_pin_set_raw()` | 모터, RS485 DE/RE, 버저, LED, 입력 | ✅ |
| ADC (SAADC) | `adc_read()` | 배터리 전압 (Tower AIN3, Link AIN7) | ✅ |
| LoRa subsys | `lora_send()`, `lora_recv()` | Tower↔Link 무선 통신 (SX1262) | ✅ |
| USB Device Stack | CDC ACM (`CONFIG_USB_DEVICE_STACK`) | Tower↔Host USB 통신 | ✅ |
| QSPI Flash | flash 드라이버 (MX25R1635) | 텔레메트리 버퍼링 | ✅ |
| I2C | `i2c_write()`, `i2c_read()` | MCP23017 IO Expander (Tower) | ✅ |
| Devicetree overlay | `.overlay` 파일 | 보드별 핀 매핑 커스터마이징 | ✅ |
| printk console | `CONFIG_PRINTK` | 디버그 출력 (UART) | ✅ |

---

## 3. Zephyr RTOS 필요성 분석

### 3.1 이미 Zephyr에 의존하는 기능 (대체 불가)

현재 동작 중인 코드가 Zephyr 커널/드라이버에 직접 의존하므로, bare-metal 전환 시 **전부 재작성** 필요.

| 기능 | Zephyr 의존도 | bare-metal 전환 비용 |
|------|:------------:|:-------------------:|
| SX1262 LoRa 드라이버 | 높음 | SPI + 레지스터 직접 제어 필요 |
| USB CDC ACM | 높음 | nRF USB HAL 직접 구현 |
| QSPI Flash | 높음 | QSPI peripheral 직접 제어 |
| Devicetree 핀 매핑 | 높음 | 하드코딩 또는 자체 추상화 |
| I2C 드라이버 | 중간 | TWIM peripheral 직접 제어 가능 |
| GPIO/ADC | 낮음 | nRF HAL로 가능 |

### 3.2 통합 펌웨어에서 필수적인 RTOS 기능

현재 드라이버 테스트는 단일 기능 검증이라 `main()` 루프 하나로 충분했지만, **통합 펌웨어 단계에서는 동시성 관리가 필수**.

| 필요 기능 | Zephyr API | 용도 | bare-metal 대안 |
|-----------|-----------|------|-----------------|
| 스레드 분리 | `k_thread_create()` | RX/TX/센서 폴링 분리 | 직접 스케줄러 구현 |
| Workqueue | `k_work`, `k_work_delayable` | 주기 폴링, 비동기 처리 | 타이머 ISR + 플래그 |
| Mutex | `k_mutex` | UART MUX 점유권 lock | 인터럽트 disable 방식 |
| 세마포어 | `k_sem` | ISR↔스레드 동기화 | volatile 플래그 |
| 타이머 | `k_timer` | TDMA 슬롯, 모터 자동 정지 | HW 타이머 직접 설정 |
| NVS/Settings | `CONFIG_NVS` | DeviceID, link_id, 키 저장 | Flash 섹터 직접 관리 |
| AES-128 | mbedTLS (내장) | LoRa 프레임 암복호 | 라이브러리 포팅 |
| Watchdog | WDT 드라이버 | hang 복구 | nRF WDT 레지스터 직접 |
| MCUboot OTA | Zephyr 생태계 | LTE/USB 경유 펌웨어 업데이트 | 매우 어려움 |

### 3.3 Tower 동시 실행 요구

Tower는 게이트웨이 역할로, 아래 작업이 **동시에** 돌아야 함:

```
┌─────────────────────────────────────────────┐
│  Thread 1: LoRa RX                          │
│   - 비콘 60s 주기 TX                         │
│   - Link 업링크 슬롯 RX (TDMA)              │
│   - ACK 처리 (1.5s 이내)                     │
├─────────────────────────────────────────────┤
│  Thread 2: RS485 폴링                        │
│   - 조도센서 30s, 카메라 60s                  │
│   - UART MUX 점유 → 폴링 → 해제             │
├─────────────────────────────────────────────┤
│  Thread 3: LTE / MQTT                        │
│   - AT command 엔진 (타임아웃 최대 180s)     │
│   - 세션 프로토콜 10단계 실행                 │
│   - 오프라인 큐 flush                        │
├─────────────────────────────────────────────┤
│  Thread 4: USB CDC 핸들러                    │
│   - Host 명령 수신/응답                       │
│   - SBC 통신 (UART MUX 경유)                 │
├─────────────────────────────────────────────┤
│  ISR: GPIO 인터럽트                           │
│   - 버튼, 진동 감지                           │
│   - k_work로 스레드에 위임                    │
└─────────────────────────────────────────────┘
```

단일 `while(1)` 루프로 이 구조를 유지하면:
- LoRa ACK 1.5s 데드라인 vs LTE AT 180s 타임아웃 → **타이밍 충돌**
- RS485 폴링 중 LoRa RX 불가 → **패킷 유실**
- 상태 기계(state machine) 복잡도 폭발

### 3.4 Link(Node) 동시 실행 요구

Link는 Tower보다 단순하지만 역시 동시성 필요:

```
┌─────────────────────────────────────────────┐
│  Thread 1: LoRa RX                          │
│   - 비콘 수신 → epoch 동기 + 슬롯 계산      │
│   - 다운링크 명령 수신 (밸브, 펌프 등)       │
│   - 텔레메트리 업링크 TX                     │
├─────────────────────────────────────────────┤
│  Thread 2: RS485 센서 폴링                   │
│   - 토양센서, 엽온센서 (텔레메트리 TX 5s 전) │
│   - 9600 baud + 300ms 타임아웃              │
├─────────────────────────────────────────────┤
│  Thread 3: 모터/밸브 타이머                   │
│   - CW/CCW 동작 시간 관리                    │
│   - 하드 리밋 2시간 cutoff (runaway 방지)    │
├─────────────────────────────────────────────┤
│  ISR: GPIO 인터럽트                           │
│   - 유량계 펄스 카운트 (GPIOTE rising edge)  │
│   - 진동 감지, 버튼                          │
└─────────────────────────────────────────────┘
```

### 3.5 결론

**Zephyr RTOS는 이 프로젝트에 필수.**

| 판단 기준 | 결론 |
|-----------|------|
| 기존 코드 의존성 | LoRa, USB CDC, QSPI 등 Zephyr 드라이버 기반으로 동작 중 |
| 동시성 요구 | Tower 4+ 스레드, Link 3+ 스레드 동시 실행 필수 |
| 타이밍 제약 | LoRa TDMA 슬롯 + LTE AT 타임아웃 공존 불가 (bare-metal) |
| 생태계 | nRF52840 + RAK4631 보드 지원, SX1262, MCUboot 등 활용 |
| 전환 비용 | bare-metal 전환 시 LoRa/USB/QSPI 드라이버 전면 재작성 |

---

## 4. 참조

- 기능 요구사항: `system/driver/FEATURE_LIST.md`
- LTE 세션 프로토콜: `system/protocol/LTE_SESSION_PROTOCOL.md`
- 회로도: `system/회로도/tower.h`, `system/회로도/link.h`
- LoRa 프로토콜: `system/protocol/REVITA_PROTOCOL.md`
