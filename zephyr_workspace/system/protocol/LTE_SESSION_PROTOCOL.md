# LTE 세션 프로토콜 — Tower ↔ Host 주기적 연결 설계

**문서 버전**: v0.1
**작성일**: 2026-04-11
**대상**: REVITA Tower ↔ Host(서버) 간 LTE 통신 세션 관리

---

## 1. 핵심 개념

### 1.1 일반 IoT와의 차이점

| 항목 | 일반 IoT (상시 연결) | REVITA (주기적 연결) |
|------|---------------------|---------------------|
| 연결 방식 | MQTT keep-alive 상시 유지 | Tower가 주기적으로 연결 → 교환 → 해제 |
| 전력 | 상시 전원 | 배터리/태양광 — LTE 모듈 전력 절감 필요 |
| 데이터 방향 | 양방향 실시간 | 단방향 배치 (연결 시 양방향 일괄 교환) |
| 명령 전달 | 즉시 push | Host가 명령 큐에 적재 → 다음 연결 시 전달 |
| 연결 주체 | 서버/디바이스 모두 가능 | **항상 Tower가 시작** |

### 1.2 세션이란?

```
Tower 슬립 ──► LTE 연결 ──► 세션 시작 ──► 데이터 교환 ──► 세션 종료 ──► LTE 해제 ──► Tower 슬립
                                │                              │
                                └──── 하나의 "세션" ────────────┘
```

**세션 = Tower가 LTE로 Host에 연결하여 데이터를 교환하는 하나의 단위**

- Tower가 세션을 시작 (connect)
- 모든 데이터 업로드 + 명령 다운로드를 한 번에 수행
- 세션 종료 후 LTE 모듈 power-down 또는 sleep

---

## 2. 세션 흐름

### 2.1 전체 시퀀스

```
    Tower                                          Host
      │                                              │
      │  ① CONNECT (LTE attach → TCP/MQTT connect)   │
      │─────────────────────────────────────────────►│
      │                                              │
      │  ② SESSION_START                             │
      │  { tower_id, session_seq, timestamp,         │
      │    link_count, battery, lte_rssi }            │
      │─────────────────────────────────────────────►│
      │                                              │
      │  ③ SESSION_ACK                               │
      │  { session_seq, pending_cmd_count }           │
      │◄─────────────────────────────────────────────│
      │                                              │
      │  ④ UPLOAD: 텔레메트리 데이터 (N건)            │
      │  [TELEMETRY_BATCH]                           │
      │─────────────────────────────────────────────►│
      │                                              │
      │  ⑤ UPLOAD_ACK                                │
      │  { received_count }                          │
      │◄─────────────────────────────────────────────│
      │                                              │
      │  ⑥ CMD_REQUEST (대기 명령 요청)               │
      │─────────────────────────────────────────────►│
      │                                              │
      │  ⑦ CMD_BATCH (Host가 큐에 쌓아둔 명령들)      │
      │  [cmd1, cmd2, ...]                           │
      │◄─────────────────────────────────────────────│
      │                                              │
      │  ⑧ CMD_ACK (수신 확인 + 즉시 실행 결과)       │
      │─────────────────────────────────────────────►│
      │                                              │
      │  ⑨ SCHEDULE (다음 연결 스케줄 협상)            │
      │  { next_connect_s, override_conditions }      │
      │◄─────────────────────────────────────────────│
      │                                              │
      │  ⑩ SESSION_END                               │
      │─────────────────────────────────────────────►│
      │                                              │
      │  (LTE disconnect → sleep)                    │
      │                                              │
```

### 2.2 각 단계 상세

#### ① CONNECT

Tower가 LTE 모듈을 켜고 네트워크에 접속합니다.

```
동작 순서:
1. LTE 모듈 power-on
2. AT+CFUN=1 (full functionality)
3. AT+CREG? → 등록 확인 (타임아웃 60s)
4. AT+CGDCONT → PDP context 활성화
5. TCP/MQTT 연결
```

#### ② SESSION_START — Tower → Host

세션 시작을 알리며, Tower의 현재 상태를 전달합니다.

```json
{
  "type": "SESSION_START",
  "tower_id": "A1B2C3D4",
  "session_seq": 142,
  "ts": 1775808000,
  "fw_ver": "1.0.3",
  "battery_mv": 3850,
  "lte_rssi": -75,
  "link_count": 2,
  "uptime_s": 86400,
  "stored_records": 24
}
```

- `session_seq`: 세션 일련번호 (Tower 재부팅 시 QSPI에서 복원)
- `stored_records`: 이번에 업로드할 텔레메트리 레코드 수

#### ③ SESSION_ACK — Host → Tower

Host가 세션을 수락하고, 대기 중인 명령 수를 알려줍니다.

```json
{
  "type": "SESSION_ACK",
  "session_seq": 142,
  "pending_cmd_count": 3,
  "server_ts": 1775808001
}
```

- Tower는 `server_ts`로 시간 보정 가능

#### ④ TELEMETRY_BATCH — Tower → Host

수집/저장해 둔 데이터를 일괄 업로드합니다.

```json
{
  "type": "TELEMETRY_BATCH",
  "tower_id": "A1B2C3D4",
  "records": [
    {
      "ts": 1775806200,
      "link_id": "01",
      "soil1_temp": 22.9, "soil1_humi": 45.2,
      "soil2_temp": 22.6, "soil2_humi": 48.7,
      "leaf_temp": 22.4,  "leaf_humi": 62.1,
      "btn": 0, "vib": 0,
      "flow1": 1523, "flow2": 892,
      "battery_mv": 3850,
      "rssi": -97, "snr": 9
    },
    {
      "ts": 1775807100,
      "link_id": "01",
      "soil1_temp": 23.1, "soil1_humi": 44.8,
      "...": "..."
    }
  ]
}
```

**데이터 흐름 (수집 → 저장 → 업로드)**:
```
Link 센서 ──LoRa──► Tower MCU ──► QSPI Flash (버퍼링)
                                        │
                        LTE 세션 시작 ──►│──► Host 업로드
                                        │
                        업로드 확인 ──►  │──► 레코드 삭제
```

#### ⑤ UPLOAD_ACK — Host → Tower

```json
{
  "type": "UPLOAD_ACK",
  "received_count": 24,
  "result": "ok"
}
```

- `result: "ok"` 수신 후에만 Tower가 QSPI에서 해당 레코드 삭제

#### ⑥ CMD_REQUEST — Tower → Host

```json
{
  "type": "CMD_REQUEST",
  "tower_id": "A1B2C3D4"
}
```

#### ⑦ CMD_BATCH — Host → Tower

Host가 세션 간에 큐에 쌓아둔 명령들을 일괄 전달합니다.

```json
{
  "type": "CMD_BATCH",
  "commands": [
    {
      "req_id": "e7c3-42",
      "target": "link:01",
      "cmd": "MOTOR_X_CW",
      "args": { "duration_s": 600 }
    },
    {
      "req_id": "e7c3-43",
      "target": "link:01",
      "cmd": "SET_PERIOD",
      "args": { "period_s": 900 }
    },
    {
      "req_id": "e7c3-44",
      "target": "tower",
      "cmd": "SET_SCHEDULE",
      "args": { "period_s": 1800 }
    }
  ]
}
```

#### ⑧ CMD_ACK — Tower → Host

```json
{
  "type": "CMD_ACK",
  "results": [
    { "req_id": "e7c3-42", "result": "queued" },
    { "req_id": "e7c3-43", "result": "queued" },
    { "req_id": "e7c3-44", "result": "ok" }
  ]
}
```

- `"ok"`: Tower 자체 명령, 즉시 적용
- `"queued"`: Link 대상 명령, 다음 LoRa 슬롯에서 전달 예정
- `"rejected"`: 유효하지 않은 명령

#### ⑨ SCHEDULE — Host → Tower

다음 연결 시점과 조건을 협상합니다.

```json
{
  "type": "SCHEDULE",
  "next_connect_s": 1800,
  "force_conditions": [
    { "trigger": "flow_delta_gt", "value": 100 },
    { "trigger": "battery_lt_mv", "value": 3200 },
    { "trigger": "vib_detected", "value": true }
  ]
}
```

- `next_connect_s`: 기본 다음 연결까지 대기 시간 (초)
- `force_conditions`: 이 조건 발생 시 스케줄 무시하고 즉시 연결

#### ⑩ SESSION_END — Tower → Host

```json
{
  "type": "SESSION_END",
  "session_seq": 142,
  "duration_ms": 4520
}
```

---

## 3. Host 측 설계

### 3.1 명령 큐 (Command Queue)

Host는 Tower가 연결되지 않은 동안 명령을 큐에 적재합니다.

```
사용자 웹 UI ──► 명령 큐 (DB/메모리) ──► 다음 세션에서 전달
     │                                        │
     │  "Motor X CW 600초" 버튼 클릭           │
     │  → CMD 레코드 생성 (pending)             │
     │                                        │
     │                          Tower 연결 ──►│──► CMD_BATCH로 전달
     │                          CMD_ACK ──►   │──► 상태 → delivered
```

| 필드 | 설명 |
|------|------|
| `req_id` | 고유 ID |
| `target` | `tower` 또는 `link:XX` |
| `cmd` | 명령 이름 |
| `args` | 명령 인자 |
| `status` | `pending` → `delivered` → `ack_ok` / `ack_fail` |
| `created_at` | 생성 시각 |
| `ttl_s` | 유효 기간 (초). 초과 시 자동 폐기 |

### 3.2 웹 UI 동작 방식

```
┌─────────────────────────────────────────────────────┐
│  REVITA Node Monitor                                │
│                                                     │
│  [Connection Status]                                │
│  ● Last session: 2026-04-11 14:30:22 (18분 전)      │
│  ● Next expected: ~14:48 (약 0분 후)                 │
│  ● Pending commands: 2                              │
│                                                     │
│  ┌─ Output Control ──┐  ┌─ Motor ─────────────────┐ │
│  │ LED    [ON][OFF]  │  │ X  [CW] [STOP] [CCW]   │ │
│  │ Buzzer [ON][OFF]  │  │ Y  [CW] [STOP] [CCW]   │ │
│  │                   │  │                         │ │
│  │ * 다음 세션에 전달 │  │ * 다음 세션에 전달       │ │
│  └───────────────────┘  └─────────────────────────┘ │
│                                                     │
│  ┌─ Sensors (마지막 수신) ──────────────────────────┐ │
│  │ 값은 마지막 세션에서 수신한 데이터               │ │
│  │ Soil #1  22.9°C  45.2%  │  Leaf  22.4°C  62.1% │ │
│  │ Soil #2  22.6°C  48.7%  │                       │ │
│  └─────────────────────────────────────────────────┘ │
│                                                     │
│  ┌─ Status / Counter ─────────────────────────────┐  │
│  │ Button ○ OFF   Vibration ○ OFF                 │  │
│  │ Flow #1  1523  Flow #2  892                    │  │
│  │ Battery  ████████░░  3.85V                     │  │
│  └────────────────────────────────────────────────┘  │
│                                                     │
│  ┌─ Session History ──────────────────────────────┐  │
│  │ 14:30 ▲ 24 records uploaded, 3 cmds delivered  │  │
│  │ 14:00 ▲ 24 records uploaded, 0 cmds delivered  │  │
│  │ 13:30 ▲ 23 records uploaded, 1 cmd delivered   │  │
│  └────────────────────────────────────────────────┘  │
│                                                     │
│  ┌─ Command Queue ────────────────────────────────┐  │
│  │ [pending] MOTOR_X_CW → link:01 (2분 전)        │  │
│  │ [pending] SET_PERIOD 900s → link:01 (5분 전)    │  │
│  └────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

**핵심 UI 원칙**:

1. **명령 = 즉시 실행이 아님**: 버튼 클릭 → 명령 큐에 적재 → "다음 세션에 전달됨" 표시
2. **센서값 = 마지막 세션의 스냅샷**: 실시간이 아닌 마지막 수신 시점 표시
3. **세션 상태 표시**: 마지막 세션 시각, 다음 예상 시각, 대기 명령 수
4. **명령 큐 가시성**: 사용자가 적재된 명령을 확인/취소 가능

### 3.3 데이터 저장

```
Host 수신 데이터 흐름:

TELEMETRY_BATCH ──► DB 저장 ──► 웹 UI 표시
                                   │
                              최신 값: 대시보드
                              이력: 차트/그래프
```

---

## 4. Tower 측 설계

### 4.1 세션 간 데이터 관리

```
[수집 주기 (예: 15분)]
    │
    ▼
Link ──LoRa──► Tower MCU
                   │
                   ▼
            QSPI Flash 링버퍼
            ┌──────────────┐
            │ record[0]    │ ← oldest
            │ record[1]    │
            │ ...          │
            │ record[N]    │ ← newest, write_ptr
            └──────────────┘
                   │
        LTE 세션 ──▼──► Host 업로드 ──► ACK 후 삭제
```

**QSPI 링버퍼 구조** (MX25R1635F, 2MB):

| 영역 | 크기 | 용도 |
|------|------|------|
| Header (0x000000) | 4 KB | write_ptr, read_ptr, session_seq, config |
| Data (0x001000) | ~2040 KB | 텔레메트리 레코드 링버퍼 |

- 레코드 1건 ≈ 64 byte → 약 32,000건 저장 가능
- 15분 주기, Link 1개 기준 → **약 333일분**

### 4.2 Tower 메인 루프

```c
while (1) {
    /* 1. LoRa 수신: Link 텔레메트리 수집 */
    if (lora_rx_available()) {
        record = decode_lora_frame();
        qspi_write_record(record);
    }

    /* 2. LoRa 송신: 대기 명령 전달 */
    if (pending_lora_cmd()) {
        lora_send_cmd(next_cmd());
    }

    /* 3. LTE 세션 스케줄 확인 */
    if (time_to_connect() || force_condition_met()) {
        lte_session_execute();
    }

    /* 4. 슬립 */
    sleep_until_next_event();
}
```

### 4.3 세션 실행 함수

```c
void lte_session_execute(void)
{
    /* ① LTE power-on + attach */
    lte_power_on();
    if (!lte_wait_registered(60000)) {
        lte_power_off();
        schedule_retry(300);  /* 5분 후 재시도 */
        return;
    }

    /* ② TCP/MQTT connect */
    mqtt_connect();

    /* ③ SESSION_START 전송 */
    send_session_start();
    wait_session_ack();

    /* ④ 텔레메트리 업로드 */
    int count = qspi_pending_count();
    send_telemetry_batch(count);
    if (wait_upload_ack() == OK) {
        qspi_mark_uploaded(count);
    }

    /* ⑤ 명령 수신 */
    send_cmd_request();
    cmd_batch = wait_cmd_batch();
    process_commands(cmd_batch);
    send_cmd_ack(cmd_batch);

    /* ⑥ 스케줄 수신 */
    schedule = wait_schedule();
    apply_schedule(schedule);

    /* ⑦ SESSION_END + 연결 해제 */
    send_session_end();
    mqtt_disconnect();
    lte_power_off();
}
```

---

## 5. 스케줄링 전략

### 5.1 기본 모드

| 모드 | 주기 | 용도 |
|------|------|------|
| Normal | 30분 | 일상 모니터링 |
| Frequent | 5분 | 관수 중, 이상 감지 시 |
| Power-save | 2시간 | 배터리 부족 시 |
| On-demand | 즉시 | 긴급 이벤트 |

### 5.2 즉시 연결 조건 (force_conditions)

Tower가 자체 판단하여 스케줄을 무시하고 즉시 연결하는 조건:

| 조건 | 임계값 | 설명 |
|------|--------|------|
| `battery_lt_mv` | 3200 | 배터리 위험 수준 |
| `vib_detected` | true | 진동 감지 → 도난/이상 |
| `flow_delta_gt` | 100 | 유량 급변 → 배관 이상 |
| `link_offline_min` | 60 | Link 60분 무응답 |
| `sensor_error` | true | 센서 읽기 연속 실패 |

### 5.3 Host → Tower 스케줄 변경

Host가 SCHEDULE 메시지로 다음 세션 주기를 조절:

```
상황: 관수 시작 명령 전달
  → Host가 next_connect_s = 300 (5분) 으로 설정
  → Tower는 5분 후 재연결하여 관수 결과 보고

상황: 모든 정상
  → Host가 next_connect_s = 1800 (30분) 으로 설정
  → 일상 모니터링 모드
```

---

## 6. 에러 처리

### 6.1 연결 실패

```
LTE attach 실패 ──► 5분 후 재시도 (최대 3회)
                          │
                     3회 실패 ──► 30분 후 재시도
                                      │
                                 계속 실패 ──► 2시간 간격 재시도
                                              + LED 경보
```

### 6.2 세션 중 연결 끊김

```
업로드 중 끊김 ──► 레코드 삭제하지 않음 (ACK 미수신)
                   다음 세션에서 재업로드

명령 수신 중 끊김 ──► 부분 수신된 명령 폐기
                      다음 세션에서 재수신

어느 단계에서든 ──► session_seq로 중복 방지
```

### 6.3 QSPI 버퍼 overflow

```
저장 레코드가 링버퍼 한계 도달 시:
  → 가장 오래된 레코드부터 덮어쓰기
  → 다음 세션에서 overflow 플래그 보고
  → Host가 누락 구간 인식
```

---

## 7. 보안 고려사항

| 항목 | 방안 |
|------|------|
| LTE 통신 암호화 | TLS 1.2 (MQTT 포트 8883) |
| Tower 인증 | Client certificate 또는 tower_id + PSK |
| 명령 무결성 | req_id + session_seq 기반 재전송 방지 |
| 데이터 무결성 | MQTT QoS 1 + UPLOAD_ACK 확인 |

---

## 8. 웹 시뮬레이션 (현재 단계) 과 실제 시스템 차이

| 항목 | 현재 시뮬레이션 | 실제 시스템 |
|------|----------------|-------------|
| 연결 | USB CDC 상시 연결 | LTE 주기적 연결 |
| 명령 | 즉시 전달/실행 | 명령 큐 → 다음 세션 전달 |
| 센서 | 시뮬레이션 고정값 | Link RS485 실측값 |
| 주기 | 실시간 조회 | 세션당 배치 수신 |

**단계적 전환 계획**:

1. **현재**: USB CDC 직접 연결 시뮬레이션 (명령 즉시 실행)
2. **다음**: 명령 큐 + 세션 히스토리 UI 추가 (USB CDC로 세션 시뮬레이션)
3. **이후**: LTE 모뎀 연동, MQTT 연결, QSPI 버퍼링 적용
4. **최종**: 실제 Link LoRa 연동 + 센서 데이터 흐름

---

*작성: Claude Code*
