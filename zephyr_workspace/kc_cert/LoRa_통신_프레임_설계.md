# LoRa Star Topology 통신 프레임 설계

## 1. 시스템 구성

| 역할 | Target | J-Link S/N | USB 콘솔 | 펌웨어 |
|------|--------|-----------|----------|--------|
| End Device (RX) | Target 1 | 1050295470 | /dev/ttyUSB0 | kc_firmware |
| Gateway (TX) | Target 2 | 1050234191 | /dev/ttyUSB2 | lora_gateway |
| Host PC | - | - | /dev/ttyUSB1 (RS485) | Web UI (Flask) |

- Gateway가 2초 간격으로 End Device에 count 값을 브로드캐스트
- End Device는 수신 데이터를 Modbus 레지스터로 노출 → Web UI에서 모니터링

---

## 2. 라디오 파라미터

| 항목 | 값 | 비고 |
|------|-----|------|
| Frequency | 922.1 MHz | 한국 ISM 대역 (920~925 MHz) |
| Bandwidth | 125 kHz | BW_125_KHZ |
| Spreading Factor | SF9 | 감도/속도 균형 |
| Coding Rate | CR 4/6 | FEC 50% 중복 |
| Preamble | 8 symbols | |
| TX Power | 14 dBm | SX1262 |
| Payload | 16 bytes | 고정 길이 |
| TX Interval | 2000 ms | Gateway 전송 주기 |

---

## 3. Over-the-Air 프레임 구조

```
┌──────────────────────────────────────────────────────────┐
│ LoRa PHY Layer (SX1262 HW 자동 처리)                      │
│                                                          │
│  Preamble (8 symbols)                                    │
│  Header (implicit mode)                                  │
│  ┌────────────────────────────────────────────────────┐  │
│  │ Application Payload (16 bytes, AES-128 암호화됨)    │  │
│  │                                                    │  │
│  │  Offset  Size  Field                               │  │
│  │  ──────  ────  ─────────────────────────────────── │  │
│  │  [0]     1B    Magic: 0xAA                         │  │
│  │  [1]     1B    Type:  0x01 (count broadcast)       │  │
│  │  [2]     1B    Sequence: 0~255 (wrapping)          │  │
│  │  [3]     1B    Flags: 0x01 (encrypted)             │  │
│  │  [4-7]   4B    Count (uint32, big-endian)          │  │
│  │  [8-11]  4B    Uptime seconds (uint32, big-endian) │  │
│  │  [12-13] 2B    CRC-16 (bytes 0~11 대상)            │  │
│  │  [14-15] 2B    Padding: 0x0000                     │  │
│  └────────────────────────────────────────────────────┘  │
│  LoRa PHY CRC (2 bytes, HW 자동 생성/검증)                │
│  FEC: Coding Rate 4/6 (전체 구간에 적용)                   │
└──────────────────────────────────────────────────────────┘
```

### 필드 상세

| Offset | Size | Field | 설명 |
|--------|------|-------|------|
| 0 | 1B | Magic | 0xAA 고정, 프레임 식별자 |
| 1 | 1B | Type | 0x01 = count broadcast |
| 2 | 1B | Sequence | 0~255 순환, 패킷 순서 추적 |
| 3 | 1B | Flags | bit0: 암호화 여부 (0x01 = encrypted) |
| 4-7 | 4B | Count | Gateway 카운터 값 (uint32, big-endian) |
| 8-11 | 4B | Uptime | Gateway 부팅 후 경과 시간 (초, uint32, big-endian) |
| 12-13 | 2B | CRC-16 | bytes [0-11]에 대한 CRC (big-endian) |
| 14-15 | 2B | Padding | 0x0000 (16바이트 정렬용) |

---

## 4. 암호화 (AES-128)

### 방식
- **AES-128 ECB 기반 XOR 스트림 암호화** (CTR-like)
- nRF52840 ECB 하드웨어 가속 사용
- 암호화와 복호화가 동일 연산 (XOR의 대칭성)

### 키 / 논스

```c
/* AES-128 Key (16 bytes): "REVITA_LORA_KEY1" */
{0x52, 0x45, 0x56, 0x49, 0x54, 0x41, 0x5F, 0x4C,
 0x4F, 0x52, 0x41, 0x5F, 0x4B, 0x45, 0x59, 0x31}

/* AES Nonce (16 bytes): "REVITA_NONCE_001" */
{0x52, 0x45, 0x56, 0x49, 0x54, 0x41, 0x5F, 0x4E,
 0x4F, 0x4E, 0x43, 0x45, 0x5F, 0x30, 0x30, 0x31}
```

### 처리 과정

```
1. Keystream 생성 (1회, 초기화 시)
   Keystream[16] = AES-ECB-Encrypt(Key, Nonce)

2. 암호화 (Gateway TX)
   Ciphertext[i] = Plaintext[i] XOR Keystream[i]   (i = 0..15)

3. 복호화 (End Device RX)
   Plaintext[i] = Ciphertext[i] XOR Keystream[i]   (동일 연산)
```

### nRF52840 ECB 하드웨어 사용

```c
/* ECB 데이터 블록 (4바이트 정렬 필수) */
struct {
    uint8_t key[16];        /* [0-15]  AES 키 */
    uint8_t cleartext[16];  /* [16-31] 논스 (입력) */
    uint8_t ciphertext[16]; /* [32-47] 키스트림 (출력) */
} __attribute__((aligned(4))) ecb_data;

NRF_ECB->ECBDATAPTR = (uint32_t)&ecb_data;
NRF_ECB->TASKS_STARTECB = 1;
/* → ecb_data.ciphertext에 키스트림 생성됨 */
```

---

## 5. Error Protection (3중 보호)

| Layer | 방식 | 위치 | 역할 |
|-------|------|------|------|
| 1. LoRa FEC | Coding Rate 4/6 | PHY 전체 | 전송 오류 정정 (50% 중복 비트) |
| 2. LoRa HW CRC | 16-bit CRC | PHY payload | 수신 무결성 검증 (HW 자동) |
| 3. App CRC-16 | Modbus poly 0xA001 | bytes [0-11] | 복호화 후 데이터 무결성 검증 |

### App CRC-16 계산

```c
/* CRC-16 (Modbus 동일 다항식: 0xA001, 초기값: 0xFFFF) */
uint16_t crc16(const uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else         crc >>= 1;
        }
    }
    return crc;
}
```

### 수신 측 검증 순서

```
1. LoRa PHY CRC 통과? (HW 자동, 실패 시 패킷 폐기)
2. 페이로드 길이 == 16? (아니면 폐기)
3. AES XOR 복호화
4. Magic == 0xAA? (아니면 폐기)
5. Type == 0x01? (아니면 폐기)
6. App CRC-16 검증 (bytes 0-11 → bytes 12-13과 비교)
7. 통과 → Count, Uptime, Seq 추출 → 레지스터 업데이트
```

---

## 6. 동작 흐름

```
Gateway (target2, S/N 191)              End Device (target1, S/N 470)
──────────────────────────              ─────────────────────────────
 main() 시작                              main() 시작
 SX1262 초기화 (TX 모드)                   SX1262 초기화 (RX 모드)
 AES keystream 생성                       AES keystream 생성
 ┌───────────────────────┐               ┌─────────────────────────┐
 │ 2초 루프:              │               │ RX 스레드 (연속 수신):    │
 │  1. Frame 생성 (16B)   │               │  1. lora_recv() 대기     │
 │  2. CRC-16 계산        │               │                         │
 │  3. AES XOR 암호화     │               │                         │
 │  4. lora_send()        │──── RF ────→  │  2. AES XOR 복호화      │
 │  5. count++, seq++     │               │  3. Magic/Type 확인     │
 │  6. k_sleep(2000ms)    │               │  4. CRC-16 검증         │
 └───────────────────────┘               │  5. Count/Uptime 추출   │
                                         │  6. 레지스터 업데이트     │
                                         └─────────────────────────┘
                                                    │
                                          Modbus RTU (RS485)
                                                    │
                                         ┌─────────────────────────┐
                                         │ Host PC (Web UI)         │
                                         │  LoRa RF 섹션에서 조회    │
                                         │  - RX Count, Errors     │
                                         │  - Last Count, Seq      │
                                         │  - RSSI, SNR            │
                                         │  - GW Uptime            │
                                         └─────────────────────────┘
```

---

## 7. Modbus 레지스터 맵 (LoRa 관련)

### 상태 레지스터 (Read: FC 0x03)

| Address | Field | 설명 |
|---------|-------|------|
| 0x0050 | lora_state | 0=IDLE, 1=RX_LISTENING |
| 0x0051 | freq | 주파수 코드 (예: 922 = 922.1MHz) |
| 0x0052 | power | TX 파워 (int16, dBm) |
| 0x0053 | rssi | 마지막 수신 RSSI (int16, dBm) |
| 0x0054 | snr | 마지막 수신 SNR (int16, x10) |
| 0x0055 | tx_cnt | 송신 카운트 |
| 0x0056 | rx_cnt | 수신 성공 카운트 |
| 0x0057 | rx_err | 수신 오류 카운트 (CRC/복호화 실패) |
| 0x0058 | last_count_lo | 마지막 수신 count (하위 16비트) |
| 0x0059 | last_count_hi | 마지막 수신 count (상위 16비트) |
| 0x005A | last_seq | 마지막 수신 sequence 번호 |
| 0x005B | last_uptime_lo | GW uptime (하위 16비트) |
| 0x005C | last_uptime_hi | GW uptime (상위 16비트) |

### 제어 레지스터 (Write: FC 0x06)

| Address | Value | 동작 |
|---------|-------|------|
| 0x0130 | 0 | LoRa RX 중지 |
| 0x0130 | 4 | LoRa RX 시작 |

---

## 8. 소스 파일

| 파일 | 설명 |
|------|------|
| `kc_firmware/src/lora.c` | End Device LoRa RX 모듈 |
| `kc_firmware/src/lora.h` | LoRa 헤더 (프레임/암호화 정의) |
| `lora_gateway/src/main.c` | Gateway TX (2초 간격 count 전송) |
| `rs485/host/app.py` | Web UI 백엔드 (LoRa 레지스터 조회) |
| `rs485/host/templates/index.html` | Web UI 프론트엔드 (LoRa RF 섹션) |

---

## 9. 빌드 및 플래시

```bash
# NCS 환경 설정
source /home/uttec/ncs/.venv/bin/activate
export ZEPHYR_BASE=/home/uttec/ncs/zephyr

# End Device (target1) 빌드 & 플래시
west build -b rak4631 kc_firmware -d build/kc_firmware --pristine
JLinkExe -USB 1050295470 -CommanderScript /tmp/flash.jlink

# Gateway (target2) 빌드 & 플래시
west build -b rak4631 lora_gateway -d build/lora_gateway --pristine
JLinkExe -USB 1050234191 -CommanderScript /tmp/flash.jlink

# Web UI 실행
cd rs485 && python3 host/app.py /dev/ttyUSB1 9600 5000
```

---

*작성: Claude Code*
*최종 업데이트: 2026-04-01*
