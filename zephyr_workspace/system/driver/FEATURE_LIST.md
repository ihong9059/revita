# REVITA System — 기능별 드라이버 요구사항

> **분석 대상**: `zephyr_workspace/` 전체 (kc_cert/, system/, lte/, loraTest/ 등)
> **출처 문서**:
> - `system/회로도/REVITA_TOWER_v1.pdf`, `tower.h`
> - `system/회로도/REVITA_LINK_v2.pdf`, `link.h`
> - `system/protocol/REVITA_PROTOCOL.md` (v0.3)
> - `system/protocol/RS485_PROTOCOL.md`
> - `kc_cert/tower/PORT_MAP.md`
> - `kc_cert/kc_firmware/src/` (현 Link 펌웨어 참조)
> **작성일**: 2026-04-07

---

## 0. 공통 인프라 (Tower / Link 동일)

| 카테고리 | 기능 | 상태 | 비고 |
|---------|------|:----:|------|
| 시스템 | Zephyr 부팅 / printk console | ✅ | Tower=`&uart0` P0.20/P0.19, Link=`&uart1` P0.16/P0.15 (다름!) |
| 시스템 | Workqueue / 스레드 분리 | 🔲 | RX/TX/sensor poll 분리 |
| 시스템 | 시스템 시각 (k_uptime) | 🔲 | epoch 동기화는 LoRa beacon으로 |
| 시스템 | Watchdog (nRF WDT) | 🔲 | hang 복구 |
| 시스템 | RTT 또는 UART log | 🔲 | log level runtime 변경 |
| 저장 | NVS / Settings 모듈 | 🔲 | DeviceID, link_id, key, period 등 |
| 보안 | AES-128 ECB (mbedTLS or PSA) | 🔲 | LoRa 프레임 암복호 |
| 보안 | CRC16-CCITT (poly 0x1021) | 🔲 | LoRa 프레임 무결성 |
| 보안 | CRC16-Modbus (poly 0xA001) | 🔲 | RS485 프레임 |
| 시간 | k_timer / k_work_delayable | 🔲 | 주기 폴링, TDMA 슬롯 |
| 부트 | DeviceID 추출 (FICR DEVICEID) | 🔲 | OP_JOIN_REQ |
| OTA | MCUboot + DFU (옵션) | 🔲 | LTE/USB 경유, LoRa 인라인은 v2 |

---

# PART A. TOWER (RAK4631 + MCP23017 + LTE + RS485 + LoRa)

> **역할**: 게이트웨이. MQTT(LTE) ↔ LoRa(Link N개) ↔ RS485(타워 직결 센서) 변환 허브.
> **회로도 핵심**: tower.h / REVITA_TOWER_v1_p1.pdf

## A1. MCU 직접 제어 (nRF52840 GPIO)

### A1.1 I2C 마스터 (P0.13 SDA / P0.14 SCL)
- [x] **I2C0 100 kHz Standard Mode** (Fast 400kHz는 RAK4631 보드에서 불안정)
  - **반드시 overlay 에 핀 명시**: `TWIM_SDA P0.13`, `TWIM_SCL P0.14`
  - 기본 board overlay 만으로는 MCP 응답 안 함 (학습됨, 2026-04-07)
- [x] MCP23017 슬레이브 (0x20) 통신 — `buzzer_test/tower` 검증
- [ ] (예비) 추가 I2C 슬레이브 확장 슬롯
- [ ] I2C 에러 복구 (recover bus on stuck SDA)

### A1.2 UART
- [x] **UART_DEBUG** (Zephyr `&uart0`, **P0.20 TX / P0.19 RX** = RAK_UART1)
  - 115200 8N1 → /dev/ttyUSB1
  - 외부 USB-UART 직결 (J-Link CDC 아님)
  - Tower 의 P0.20 은 동시에 TMUX1574 → RS485 트랜시버로도 라우팅됨
  - `system/tower` 디버그 스켈레톤에서 검증
- [ ] **UART_LTE** — 회로도상 RAK_UART2 (P0.16/P0.15) → LTE 모듈
  - Zephyr `&uart1` 사용 예정
  - kc_cert/system/lte 패턴 참조
- [x] **UART_RS485** — uart0 (P0.20/P0.19) 공유 (디버그 console 과 같은 핀)
  - 115200 8N1 (kc_cert/tower 검증값)
  - DE=P0.17, RE#=P0.21 (직접 GPIO)
  - DE=LOW 일 때 트랜시버 high-Z → 디버그 printk 가 wire 에 안 나감
  - **HW 의심으로 wire 출력 미확인** (rs485_test/gateway, 다음 세션 점검)

### A1.3 GPIO (직접 핀)
- [x] `PIN_RS485_DE` (P0.17) — 출력, RS485 송신 enable
- [x] `PIN_RS485_RE_N` (P0.21) — 출력, RS485 수신 enable (active low)
- [ ] `PIN_LTE_RI` (P0.10/NFC2) — 입력 인터럽트, **UICR.NFCPINS off 필요**
- [ ] `PIN_LTE_DTR` (P0.09/NFC1) — 출력, **UICR.NFCPINS off 필요**
- [ ] `PIN_BTN` (P1.02/SW2) — 입력, 사용자 버튼 (디바운스, 짧게/길게 구분)
- [ ] `PIN_VIB_SENSE` (P1.04/LED2) — 입력 인터럽트, 진동/도난 감지
- [ ] WDT kick

### A1.4 ADC (SAADC)
- [x] **`ADC_CH_BAT` (AIN3 / P0.05)** — `adc_test/tower` 검증
  - 12-bit, Gain 1/6, Internal 0.6V ref
  - 분압 1M / 220k → ×5.545
- [ ] 평균/필터링 (이동평균 N=8)
- [ ] 저전압 임계치 → 이벤트 발행

### A1.5 QSPI (NOR Flash MX25R1635, 16Mbit)
- [x] **QSPI 드라이버 활성화** — `qspi_test/tower` 검증
- [x] **셀프 테스트** (erase / write / read / verify) — PASS
- [ ] LittleFS 또는 NVS 마운트
- [ ] 용도: 텔레메트리 버퍼링, MQTT 미연결 시 큐, 로그, 설정값

---

## A2. MCP23017 I/O Expander (0x20) — 보조 GPIO 16개

### A2.1 드라이버 베이스
- [x] I2C 초기화 + IODIR 설정 — `buzzer_test/tower` 검증
- [x] OLAT 캐시 (read-modify-write 방지)
- [x] 단일 비트 set/clr (mask 기반)
- [ ] GPA/GPB 일괄 read
- [ ] IPOL / IOCON 고급 설정

### A2.2 GPA 매핑 (전원 / LTE / MUX / 카메라)
- [ ] `GPA0 = LTE_RST#` — LTE 모뎀 리셋 (active low, 500 ms 펄스)
- [x] `GPA2 = 12_14V_EN` — 12~14 V 부스트 (buzzer_test 에서 HIGH 검증)
- [~] `GPA3 = MUX_SEL` — UART MUX 채널 (코드 작성, wire 미검증)
- [~] `GPA4 = MUX_EN#` — UART MUX enable (코드 작성, wire 미검증)
- [ ] `GPA5 = 3V3_EN` — 3.3 V 레일 enable (코드 작성, 부하 측정 필요)
- [ ] `GPA6 = 5V_EN` — 5 V 레일 enable
- [ ] `GPA7 = CAM_EN` — 카메라 모듈 전원

### A2.3 GPB 매핑 (보조 리셋 / 출력)
- [ ] `GPB0 = SBC_RST#` — SBC(Luckfox) 리셋 (active low)
- [x] **`GPB1 = BUZZER_EN`** — `buzzer_test/tower` 검증 ✅
- [ ] `GPB6 = LED_EN` — 외부 LED 출력 (R45 470Ω 직렬)

### A2.4 안전 시퀀스
- [ ] 부팅 시 모든 RST# = HIGH (해제), 모든 EN = LOW (꺼짐), MUX_EN# = HIGH
- [ ] 단계적 power-up: 3V3 → 5V → 12V (각 사이 안정화 지연)
- [ ] 단계적 power-down: 역순

---

## A3. UART MUX (TMUX1574) — RAK_UART2 / RS485 / SBC 분기

### A3.1 토폴로지 (회로도 PART_3)
- TMUX1574 (Dual SPDT), `MUX_SEL` + `MUX_EN#` 로 제어
- A 채널: RAK ↔ RS485 트랜시버
- B 채널: SBC ↔ RS485 트랜시버
- 동시에 한쪽만 RS485 점유 가능

### A3.2 드라이버 기능
- [ ] `mux_select(MASTER_RAK | MASTER_SBC)`
- [ ] 소유권 교체 절차
  1. 현재 소유자 TX flush 대기
  2. `MUX_EN# = HIGH` (mute)
  3. `MUX_SEL` 토글
  4. DE/RE# 라인도 동기화 (RAK 측 RE#)
  5. `MUX_EN# = LOW` (unmute)
  6. ≥ 1 ms 안정화 대기
- [ ] 점유권 lock (k_mutex) — RS485 폴링과 SBC 통신 충돌 방지
- [ ] 타임아웃 시 강제 RAK 회수

---

## A4. LTE 모뎀 (Sierra Wireless RC76xx) — UART2 + AT command

### A4.1 물리 계층
- [ ] UART2 115200 8N1, no flow control
- [ ] RX 인터럽트 + ring buffer (≥ 512 byte)
- [ ] LTE_RST# 펄스 제어 (MCP23017 GPA0)
- [ ] LTE_DTR / LTE_RI GPIO (NFCPINS off)

### A4.2 AT command 엔진
- [ ] `at_cmd(req, rsp_buf, expect, timeout)` — 한 번에 한 명령
- [ ] 종결자 파싱: `OK` / `ERROR` / `+CME ERROR:` / `+CMS ERROR:`
- [ ] URC 분리 파서: `+CREG:`, `+CSQ:`, `+CGEV:`, `+KMQTT_DATA:` 등
- [ ] 타임아웃 등급: 일반 2 s / 네트워크 30 s / `AT+COPS=?` 180 s

### A4.3 모뎀 라이프사이클
- [ ] 부팅 시 RC76xx 모델/SKU 확인 (`ATI`, `AT!SKU`)
- [ ] SIM 확인 (`AT+CPIN?`)
- [ ] 네트워크 등록 감시 (`AT+CREG?`, `AT+CGATT?`)
- [ ] 신호 품질 (`AT+CSQ`) — 15 s 주기
- [ ] APN 설정 (`AT+CGDCONT=1,"IP","<apn>"`)
- [ ] 에러 복구: AT 무응답 3회 → `LTE_RST#` 펄스 → 재초기화

### A4.4 데이터 경로 (선택)
- [ ] **모드 A**: SBC가 PPP/MQTT 담당, MCU는 모니터링만
- [ ] **모드 B**: SBC OFF 시 MCU 자체 MQTT (`+KTCPCFG`, `+KMQTT*`)
- [ ] (옵션) GNSS 활성화 (`AT!GNSSCONFIG?`)

---

## A5. SBC (Luckfox / RPi) 인터페이스

### A5.1 USB CDC (USB0)
- [x] **MCU USB CDC ACM** — `usb_test/tower` 검증
  - VID=0x2FE3 / PID=0x0100, host /dev/ttyACMx
  - 매 2초 카운터 송신 + RX echo
  - Tower 의 MCU_USB_D_P/D_N → USB 커넥터
- [ ] (Linux 측) SBC 가 PPP/MQTT 담당
- [ ] MCU 측 모니터링: SBC heartbeat 확인

### A5.2 UART (UART_MUX 경유)
- [ ] MCU ↔ SBC UART (RS485 비점유 시간대)
- [ ] 프로토콜: TBD (단순 텍스트 명령? 자체 binary?)
- [ ] **추후 정리** — REVITA_PROTOCOL.md §4 참조

### A5.3 SBC 전원/리셋
- [ ] `GPB0 = SBC_RST#` (active low) 펄스
- [ ] `GPA5 = 3V3_EN`, `GPA6 = 5V_EN` (SBC 전원 라인 가정)
- [ ] watchdog: SBC heartbeat 60 s 실패 → 리셋

---

## A6. RS485 (Modbus RTU 마스터) — Tower 직결 센서

### A6.1 물리 계층
- [~] **uart0 (P0.20/P0.19)** — RAK_UART1 → MUX → 트랜시버 (NOT uart1!)
  - 코드 작성됨 (`rs485_test/gateway`), wire HW 검증 미해결
  - DE=P0.17, RE#=P0.21
- [x] DE/RE# 동기 토글 패턴 학습 (TX 후 2ms shift register flush)
- [ ] 3.5 char inter-frame gap
- [ ] CRC16-Modbus (poly 0xA001)

### A6.2 Modbus 마스터 엔진
- [ ] `mb_read_holding(slave, addr, count, buf, timeout)` — FC=0x03
- [ ] 응답 파싱 + CRC 검증
- [ ] 타임아웃 300 ms, 3회 재시도
- [ ] (선택) FC=0x06 / 0x10 (write) — 현 단계 불필요

### A6.3 센서 스캐너
- [ ] **조도 센서 0x03 / 0x04** — addr=0x0002, 2 reg → 32-bit lux ÷ 1000
  - 두 채널 모두 polling (단일 모델인지 확인 후 통합/분리)
- [ ] **카메라/분광 0x11** *(TBD: 인터페이스 확인 필요)*
- [ ] 폴링 주기: 조도 30 s, 카메라 60 s
- [ ] 센서 unreachable → MCP23017 BUZZER로 alert + MQTT event 업링크

### A6.4 통합
- [ ] UART_MUX 점유권 획득 → polling → 해제 (SBC 사용 시간 보장)
- [ ] 결과를 Tower 텔레메트리 캐시에 저장
- [ ] MQTT 토픽 `tower/<id>/telemetry/sensors` 로 발행

---

## A7. LoRa (SX1262) — RVT-LoRa v1 hub

### A7.1 PHY/MAC 파라미터 (REVITA_PROTOCOL §5.1)
- 922.1 MHz / BW 125 kHz / SF9 운용 (SF12 fallback) / CR 4/6 / 14 dBm
- Sync word 0x12, Preamble 8, Explicit header, CRC on
- Payload **16 byte 고정** (AES-128 단일 블록)
- Duty cycle ≤ 1 % (KR ISM)

### A7.2 드라이버 기능
- [x] **Zephyr LoRa subsys** (`lora_send`, `lora_recv`) — `lora_test/gateway` 검증
- [x] TX/RX 전환 + 5초 timeout 수신
- [x] RSSI / SNR 측정 (-46 ~ -65 dBm, +5~+8)
- [ ] 듀티 사이클 회계 (한 시간 윈도우 누적, ≥ 99% 휴식)
- [ ] RX continuous mode (현재는 수동 reconfigure)
- [ ] (옵션) listen-before-talk

### A7.3 RVT-LoRa Hub 로직 (Tower = node 0x00)
- [ ] AES-128 ECB encrypt/decrypt (16 B 블록)
- [ ] CRC16-CCITT 검증 (평문 0..13 → 14..15)
- [ ] **TDMA-lite 스케줄러** (60 s 비콘 사이클)
  - [ ] `OP_BEACON` 송신 (broadcast, epoch 포함)
  - [ ] Link 업링크 슬롯 RX (`beacon + link_id × 2 s`)
  - [ ] 다운링크 창 (`beacon + 0.2~1.8 s`)
- [ ] 중복 제거 (최근 (SRC, SEQ) 8개 캐시)
- [ ] ACK 처리 (`REQ=1` 시 1.5 s 내 ACK, 최대 3회 재전송)
- [ ] **JOIN 처리** (`OP_JOIN_REQ` → `link_id` 할당 → `OP_JOIN_ACK`)
  - link_id ↔ DeviceID 매핑 NVS 저장
- [ ] 텔레메트리 수집 (`OP_TELEMETRY` 10 byte 파싱)
- [ ] 이벤트 수집 (`OP_EVENT`)
- [ ] 명령 송신 (`OP_VALVE_SET`, `OP_PUMP_SET`, `OP_GET_STATE`, `OP_SET_PERIOD`, `OP_FLOW_RESET`, `OP_REBOOT`)

### A7.4 텔레메트리 변환
- [ ] LoRa 10 byte → JSON 변환 (REVITA_PROTOCOL §3.3)
- [ ] `flow_delta` 누적 → `flow_count` 절대값 (link_id별 카운터)

---

## A8. MQTT 게이트웨이 (LoRa ↔ MQTT 매핑)

### A8.1 클라이언트
- [ ] LTE TCP / TLS 1.2 (운용 8883, 개발 1883)
- [ ] QoS 1, clean session=false, keep-alive 120 s
- [ ] LWT: `revita/tower/<tower_id>/status = offline` (retained)

### A8.2 토픽 (REVITA_PROTOCOL §3.2)
- [ ] `revita/tower/<tower_id>/{status, telemetry/sensors, telemetry/system, event, cmd, cmd/ack}`
- [ ] `revita/link/<tower_id>/<link_id>/{status, telemetry, event, cmd, cmd/ack}`

### A8.3 매핑 (REVITA_PROTOCOL §7)
- [ ] Downlink: MQTT `cmd` JSON → LoRa OP (cmd ↔ OP 테이블)
- [ ] Uplink: LoRa OP → MQTT 토픽
- [ ] `req_id ↔ (link_id, lora_seq)` 매핑 테이블
- [ ] cmd/ack 발행 (ok/err/timeout/unreachable)

### A8.4 오프라인 큐
- [ ] LTE 미연결 시 텔레메트리 → QSPI 큐 적재
- [ ] 재연결 시 일괄 flush (오래된 것 먼저)

---

## A9. Tower 운영 기능

### A9.1 로컬 IO
- [ ] 부저 패턴 (boot, alarm, ack)
- [ ] LED 패턴 (정상/네트워크 끊김/에러)
- [ ] BTN 짧게 = 상태 보고 강제 / 길게 = 진단 모드
- [ ] 진동 감지 (VIB_SENSE) → 도난 경보 이벤트

### A9.2 자가진단 / KC 인증 잔존 기능
- [ ] Power-on self test (I2C / Flash / RS485 loopback / LoRa idle)
- [ ] RF Test 모드 (kc_cert/lora_tx_test 참조)
- [ ] 결과를 부저/LED + MQTT event로 보고

### A9.3 시간/스케줄
- [ ] LTE → epoch 동기 (`AT+CCLK?`)
- [ ] LoRa 비콘으로 epoch 전파
- [ ] 일중 스케줄 (예: 야간 저전력 모드, 새벽 진단)

---

# PART B. LINK (RAK4631 + RS485 + 관수/유량/모터)

> **역할**: 필지 단위 센싱 + 관수 액추에이터. LoRa로 Tower와 통신.
> **회로도 핵심**: link.h / REVITA_LINK_v2.pdf

## B1. MCU 직접 제어 (nRF52840 GPIO)

### B1.1 UART
- [x] **UART_DEBUG** — Zephyr `&uart1`, **TX = P0.16 / RX = P0.15** (RAK_UART2)
  - 115200 8N1, ttyUSB0 (사용자 cutecom)
  - **Tower와 다른 매핑** (Tower는 `&uart0` + P0.20/P0.19). 2026-04-07 양방향 확정.
  - TX/RX 모두 `system/driver/lora_test/node`에서 검증 완료
  - 주의: kc_cert/system/link 과거 overlay는 RX=P0.02로 잘못 잡혀있었음 (TX만 사용했기에 문제 안 됨)
- [x] **UART_RS485** — Zephyr `&uart0`, **P0.20 TX / P0.19 RX** @ 9600 8N1
  - DE=P1.04, RE#=P1.03 (직접 GPIO, Tower 와 핀 다름!)
  - 12V_EN=P0.17 (직접 GPIO) HIGH 필요
  - **TX→RX 전환 시 `k_busy_wait(2000)` (2ms shift register flush) 필수**
  - `rs485_test/node` wire 검증 완료 ✅

### B1.2 GPIO (직접 핀, link.h 기준)

**X 채널 (모터/액추에이터 추정)**
- [ ] `PIN_X_EN_A` (P0.14) — 출력
- [ ] `PIN_X_EN_B` (P0.13) — 출력
- [ ] `PIN_X_EN_P2` (P0.04) — 출력 (또는 ADC AIN2)

**Y 채널**
- [ ] `PIN_Y_EN_A` (P0.25) — 출력
- [ ] `PIN_Y_EN_B` (P1.01) — 출력
- [ ] `PIN_Y_EN_P2` (P1.02) — 출력

**RS485 방향**
- [x] `PIN_RS485_DE` (P1.04) — 출력, `rs485_test/node` 검증
- [x] `PIN_RS485_RE_N` (P1.03) — 출력, 검증

**전원/액추에이터 enable**
- [x] `PIN_12V_EN` (P0.17) — 출력, `buzzer_test/node` + `rs485_test/node` 검증
- [x] `PIN_BUZZER_EN` (P0.24) — 출력, `buzzer_test/node` 검증 ✅

**입력**
- [ ] `PIN_VIB_SENSE` (P0.21) — 입력 인터럽트
- [ ] `PIN_BTN` (P0.05/AIN3) — 입력 (디바운스)
- [ ] `PIN_DIO_X` (P0.10/NFC2) — 입력 인터럽트, **UICR.NFCPINS off 필요**
- [ ] `PIN_DIO_Y` (P0.09/NFC1) — 입력 인터럽트, **UICR.NFCPINS off 필요**

### B1.3 ADC (SAADC)
- [x] **`ADC_CH_BAT` (AIN7 / P0.31)** — `adc_test/node` 검증
  - 12-bit, Gain 1/6, Internal 0.6V ref
  - 분압 1M / 1M → ×2
- [ ] `ADC_CH_BTN` (AIN3 / P0.05) — 버튼 (디지털도 가능)
- [ ] `ADC_CH_X_EN_P2` (AIN2 / P0.04) — X채널 P2 모니터링
- [ ] 평균 / 이동평균 N=8

### B1.4 QSPI (NOR Flash, 모듈/외장)
- [x] **QSPI 드라이버 활성화** — `qspi_test/node` 검증
- [x] **셀프 테스트** (erase / write / read / verify) — PASS
- [ ] LittleFS 또는 NVS 마운트
- [ ] 용도: link_id, 키, 미전송 텔레메트리 큐, 유량 누적 백업

---

## B2. RS485 센서 마스터 (Modbus RTU)

### B2.1 물리 계층
- [x] **uart0 9600 8N1** (NOT uart2 — uart2 라벨은 회로도, Zephyr 에서는 uart0)
- [x] **DE/RE# (P1.04/P1.03) 동기 토글** + 2ms shift register flush
- [x] 12V_EN (P0.17) HIGH 트랜시버 전원
- [x] wire 송신 검증 ✅ (`rs485_test/node`)
- [ ] 3.5 char inter-frame gap (Modbus 프레임 분리)
- [ ] CRC16-Modbus (poly 0xA001)

### B2.2 센서 스캐너 (RS485_PROTOCOL.md 기준)
- [ ] **토양 센서 0x01** — addr=0x0000, 2 reg
  - reg[0] = 습도 hi/lo → `humid_pct = raw / 10`
  - reg[1] = 온도 hi/lo → `temp_c = (int16)raw / 10`
- [ ] **엽온 센서 0x02** — addr=0x0000, 2 reg
  - reg[0] = 습도 hi/lo → `leaf_humid_pct = raw / 10` ★신규
  - reg[1] = 온도 hi/lo → `leaf_temp_c = (int16)raw / 10`
- [ ] **NPK 센서 0x05** *(옵션 부착)* — addr=0x001E, 3 reg
  - 단위/스케일 TBD
- [ ] 폴링 주기: 텔레메트리 송신 −5 s (LoRa TX 직전 최신값)
- [ ] 실패 시 `state.err_mask` 비트 set:
  - 토양 → b2
  - 엽온 → b3
- [ ] 3회 연속 실패 → `OP_EVENT` 업링크

---

## B3. 관수 모터 (X/Y 채널, 3선식 추정)

### B3.1 채널 정의
- 회로도상 X/Y 두 채널 — 밸브 또는 펌프 추정
- 각 채널: `EN_A` + `EN_B` + `EN_P2` (3선식 H-bridge 또는 모터 드라이버 control)

### B3.2 드라이버 기능
- [ ] `valve_open(channel)` / `valve_close(channel)` / `valve_stop(channel)`
  - kc_firmware/drivers.h의 `drv_valve_cw/ccw/stop` 패턴 참조
- [ ] CW/CCW 시퀀스 (EN_A, EN_B, EN_P2 조합)
- [ ] 동작 시간 타이머 (duration_s) → 자동 stop
- [ ] **하드 리밋**: duration_s=0 무한 동작 시 최대 2시간 cutoff (runaway 방지)
- [ ] 과전류/스톨 감지 (P2 라인 ADC 모니터링?)
- [ ] 12V 전원 enable과 동기화 (`LINK_12V_ON()` 선행)

### B3.3 OP 매핑 (LoRa)
- [ ] `OP_VALVE_SET (0x40)` → state(open/close), duration_s
- [ ] `OP_PUMP_SET (0x41)` → state(on/off), duration_s
- [ ] 결과 반영: 다음 텔레메트리 `state.b0` (valve), `state.b1` (pump)

---

## B4. 유량계 (DIO pulse 카운트)

### B4.1 입력
- [ ] `PIN_DIO_X` 또는 `PIN_DIO_Y` 중 하나가 유량계 입력 (배선 확인 필요)
- [ ] GPIOTE 인터럽트 (rising edge)
- [ ] 디바운스: HW RC + SW 5 ms re-check

### B4.2 카운터
- [ ] u16 `flow_count` (wrap)
- [ ] u8 `flow_delta` (텔레메트리 송신 시 reset)
- [ ] `OP_FLOW_RESET (0x43)` 수신 시 0으로

### B4.3 변환
- [ ] L/pulse 환산은 서버 측 (MCU는 raw count만)
- [ ] 8-bit delta overflow 시 `state.b4` set

---

## B5. LoRa Leaf (RVT-LoRa v1, node_id = 0x01..0xFE)

### B5.1 PHY (Tower와 동일)
- [x] **Zephyr LoRa subsys** — `lora_test/node` echo 검증 (SF12, RSSI -46~-65)
- [x] TX→RX 전환, warmup TX 패턴 (radio 안정화)
- [ ] AES-128 ECB + CRC16-CCITT (REVITA 프로토콜 단계)

### B5.2 MAC 로직
- [ ] `OP_JOIN_REQ` 송신 (DeviceID 포함) — 부팅 후 link_id 미할당 시
- [ ] `OP_JOIN_ACK` 수신 → link_id NVS 저장
- [ ] `OP_BEACON` 수신 → epoch 동기, 자기 슬롯 계산
- [ ] **자기 슬롯에서만 송신** (`beacon_rx + link_id × 2 s`)
- [ ] `OP_TELEMETRY` 주기 송신 (기본 5 분 = 비콘 5회마다)
- [ ] `OP_EVENT` 즉시 송신 (alarm 발생 시)
- [ ] `OP_GET_STATE` 수신 → `OP_STATE` 응답
- [ ] 다운링크 명령 처리: VALVE_SET/PUMP_SET/SET_PERIOD/FLOW_RESET/REBOOT
- [ ] ACK 송수신 (`REQ=1` 시), 중복 제거
- [ ] **저전력**: 슬롯 외 SLEEP, 비콘 시각 ±100 ms drift 허용

### B5.3 텔레메트리 빌더 (struct rvt_telemetry, 10 byte)
- [ ] `batt_mv` (ADC)
- [ ] `soil_temp_cx10`, `soil_moist_pct` (RS485 0x01)
- [ ] `leaf_temp_cx10`, `leaf_humid_pct` (RS485 0x02) ★신규
- [ ] `flow_delta` (DIO)
- [ ] `state` 비트:
  - b0 = valve, b1 = pump, b2~b4 = err_mask, b5~b7 = reserved

---

## B6. Link 운영 기능

### B6.1 로컬 IO
- [ ] 부저 (`LINK_BUZZER_ON/OFF`) — 관수 시작/종료, 알람
- [ ] 진동 감지 (VIB_SENSE) → 도난 경보 → `OP_EVENT`
- [ ] BTN 짧게 = 강제 텔레메트리 / 길게 = 진단 모드

### B6.2 전원 관리
- [ ] `LINK_12V_ON/OFF` — 모터 동작 시에만 ON
- [ ] 평상시 모든 액추에이터 OFF, MCU만 활성
- [ ] 저전력 sleep (k_sleep + LoRa RX wakeup)
- [ ] 배터리 임계치 도달 시 액추에이터 동작 거부 → 이벤트

### B6.3 자가진단
- [ ] 부팅 시 RS485 sensor probe (각 slave_ID ping)
- [ ] Flash self-test
- [ ] LoRa join 성공 여부
- [ ] 결과 → `OP_EVENT` 또는 부저 패턴

---

# PART C. 우선순위 매트릭스 (구현 진척)

| 순위 | Tower 기능 | Link 기능 | 상태 | 비고 |
|:----:|-----------|-----------|:----:|------|
| 1 | A1.2 UART_DEBUG | B1.1 UART_DEBUG | ✅ | 양쪽 핀 매핑 확정 |
| 2 | A2 MCP23017 driver | B1.2 GPIO 베이스 | ✅ | I2C 핀 명시 + 100kHz, GPIO 검증 |
| 3 | A1.5 QSPI | B1.4 QSPI | ✅ | MX25R1635F erase/write/read PASS |
| 3' | A1.4 ADC | B1.3 ADC | ✅ | 배터리 전압 측정 |
| 3'' | A2.3 BUZZER (MCP) | B1.2 BUZZER (직접) | ✅ | 양쪽 동작 |
| 3''' | A5.1 USB CDC | — | ✅ | Tower USB CDC ACM |
| 4 | A6 RS485 (Modbus 마스터) | B2 RS485 센서 | 🟡 | Link wire ✅, Tower wire HW 의심 |
| 5 | A7.1~7.2 LoRa PHY | B5.1 LoRa PHY | ✅ | Gateway↔Node echo, SF12 검증 |
| 6 | A7.3 RVT-LoRa Hub | B5.2 RVT-LoRa Leaf | 🔲 | JOIN/BEACON/TELEMETRY |
| 7 | — | B3 관수 모터 | 🔲 | 안전 cutoff 포함 |
| 8 | — | B4 유량계 | 🔲 | NFC 핀 GPIO 재할당 필요 |
| 9 | A4 LTE 모뎀 | — | 🔲 | kc_cert/system/lte 참조 |
| 10 | A8 MQTT 게이트웨이 | — | 🔲 | LTE 안정화 후 |
| 11 | A3 UART MUX (SBC 협조) | — | 🔲 | SBC 측 정의 후 |
| 12 | A9 / B6 운영 기능 | A9 / B6 운영 기능 | 🔲 | 마무리 |
| - | A1.3 BTN/VIB_SENSE | B1.2 BTN/VIB_SENSE | 🔲 | 입력 인터럽트 |

---

# PART D. 공유 가능한 드라이버 모듈 (코드 재사용)

`system/driver/` 하위에 두면 Tower/Link 양쪽 빌드에서 include 가능한 모듈:

| 모듈 | 파일 (제안) | 사용처 |
|------|------------|--------|
| Modbus RTU 마스터 | `modbus_master.{c,h}` | Tower(A6), Link(B2) |
| CRC16 (Modbus + CCITT) | `crc16.{c,h}` | LoRa, RS485 |
| AES-128 ECB 래퍼 | `rvt_crypto.{c,h}` | LoRa 양쪽 |
| RVT-LoRa 프레임 빌더/파서 | `rvt_lora.{c,h}` | Tower hub + Link leaf |
| RVT 텔레메트리 struct | `rvt_telemetry.h` | 양쪽 |
| RAK4631 RS485 half-duplex 헬퍼 | `rs485_uart.{c,h}` | 양쪽 (DE/RE# 핀만 보드별로) |
| NVS 설정 wrapper | `rvt_nvs.{c,h}` | 양쪽 |
| 부저/LED 패턴 엔진 | `indicator.{c,h}` | 양쪽 |

**보드별 분리해야 하는 것**:
- 핀 매핑 (`tower_board.h`, `link_board.h`)
- MCP23017 드라이버 (Tower 전용)
- LTE/MQTT (Tower 전용)
- 관수/유량 (Link 전용)

---

# PART E. 미해결 / 확인 필요 사항

## 해결됨 (2026-04-07)
- [x] **Tower 디버그 UART**: `&uart0` P0.20/P0.19 (RAK_UART1) 사용 → ttyUSB1
  LTE 는 RAK_UART2 (P0.16/P0.15) → Zephyr `&uart1` 별도 매핑 예정
- [x] **Link 디버그 UART**: `&uart1` P0.16/P0.15 → ttyUSB0 (Tower 와 다름)
- [x] **Tower I2C0 핀 명시**: overlay 에서 SDA P0.13 / SCL P0.14, 100kHz Standard
- [x] **9600 RS485 TX timing**: 마지막 byte shift register flush 위해 2ms busy_wait

## HW 검증 미해결
- [ ] **Tower RS485 wire 출력**: 코드 정리됨, A/B 라인에 신호 미확인 — 트랜시버/MUX/배선 점검 필요
- [ ] **NFC 핀 GPIO 재할당** (`CONFIG_NFCT_PINS_AS_GPIOS=y` + UICR write):
  Tower (P0.09/P0.10 = LTE_DTR/LTE_RI), Link (P0.09/P0.10 = DIO_Y/DIO_X)

## 사양 확인 필요
- [ ] **카메라 인터페이스**: RS485 인지 별도(SPI/I2C/USB) 인지 회로도 재확인
- [ ] **X/Y 채널 매핑**: Link X/Y 가 정확히 무엇을 제어하는지 (밸브 vs 펌프)
- [ ] **유량계 입력 핀**: DIO_X / DIO_Y 중 어느 쪽인지
- [ ] **RS485 센서 baudrate**: 9600 가정 — 센서 사양서로 확정
- [ ] **NPK 단위/스케일** (RS485_PROTOCOL.md §6)
- [ ] **MCP23017 GPB6 LED**: 단일 LED 인지 다채널인지

---

# PART F. 구현 완료 프로젝트 (2026-04-07 기준)

| 프로젝트 | 경로 | 검증 |
|---------|------|:----:|
| Tower 디버그 스켈레톤 | `system/tower/` | ✅ |
| LoRa Gateway | `system/driver/lora_test/gateway/` | ✅ |
| LoRa Node (echo) | `system/driver/lora_test/node/` | ✅ |
| RS485 Node (wire 송신) | `system/driver/rs485_test/node/` | ✅ |
| RS485 Gateway (wire 송신) | `system/driver/rs485_test/gateway/` | 🟡 HW 의심 |
| Tower USB CDC ACM | `system/driver/usb_test/tower/` | ✅ |
| QSPI Tower | `system/driver/qspi_test/tower/` | ✅ |
| QSPI Node | `system/driver/qspi_test/node/` | ✅ |
| Buzzer Tower (MCP23017) | `system/driver/buzzer_test/tower/` | ✅ |
| Buzzer Node (직접 GPIO) | `system/driver/buzzer_test/node/` | ✅ |
| ADC Tower (배터리) | `system/driver/adc_test/tower/` | ✅ |
| ADC Node (배터리) | `system/driver/adc_test/node/` | ✅ |

---

*작성: Claude Code*
*최초 작성: 2026-04-07*
*최종 업데이트: 2026-04-07 (드라이버 검증 결과 반영)*
