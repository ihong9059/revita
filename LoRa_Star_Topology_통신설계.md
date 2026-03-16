# RAK4630 LoRa Star Topology 통신 설계

## 개요

| 항목 | 값 |
|------|-----|
| 디바이스 | RAK4630 (nRF52840 + SX1262) |
| RTOS | Zephyr |
| 토폴로지 | Star (중앙 허브 + 다수 노드) |
| Payload 크기 | 16 bytes |

---

## 1. 권장 암호화 방식: AES-128-CCM

### 1.1 선택 이유

| 장점 | 설명 |
|------|------|
| **블록 크기 일치** | AES 블록 크기(16바이트)가 payload(16바이트)와 정확히 일치 |
| **인증 + 암호화 통합** | CCM 모드는 암호화와 메시지 인증(MAC)을 한번에 제공 |
| **LoRaWAN 표준** | LoRaWAN에서 검증된 방식으로 신뢰성 확보 |
| **낮은 오버헤드** | 작은 메시지에 최적화, 최소 4바이트 MAC 태그 |
| **Zephyr 지원** | Zephyr의 TinyCrypt/mbedTLS에서 네이티브 지원 |
| **하드웨어 가속** | nRF52840의 AES 하드웨어 가속기 활용 가능 |

### 1.2 다른 방식과의 비교

| 방식 | 장점 | 단점 | 적합성 |
|------|------|------|--------|
| **AES-128-CCM** | 인증+암호화, 작은 오버헤드, HW 가속 | - | ✅ 최적 |
| AES-128-GCM | 빠른 처리, 병렬화 가능 | 96비트 nonce 고정, 약간 큰 오버헤드 | ⭕ 양호 |
| AES-128-CTR | 간단, 스트림 암호 | 별도 MAC 필요 (HMAC 추가) | △ 보통 |
| ChaCha20-Poly1305 | 소프트웨어 효율적 | AES HW 가속 활용 불가 | △ 보통 |

### 1.3 CCM 모드 구조

```
┌─────────────────────────────────────────────────────────┐
│                    CCM 패킷 구조                         │
├─────────────┬─────────────────────┬────────────────────┤
│   Nonce     │   Encrypted Data    │   MAC Tag          │
│  (13 bytes) │    (16 bytes)       │   (4-8 bytes)      │
└─────────────┴─────────────────────┴────────────────────┘
```

**Nonce 구성 (13바이트)**:
- Device ID: 4바이트
- Frame Counter: 4바이트
- Direction Flag: 1바이트 (0=uplink, 1=downlink)
- Reserved: 4바이트

### 1.4 키 관리 (Star Topology)

```
        ┌─────────┐
        │   Hub   │  Master Key 보유
        │(Gateway)│  모든 노드 키 관리
        └────┬────┘
             │
    ┌────────┼────────┐
    │        │        │
┌───┴───┐┌───┴───┐┌───┴───┐
│Node 1 ││Node 2 ││Node 3 │  각 노드별 고유 키
└───────┘└───────┘└───────┘
```

**키 구조**:
- **Network Key**: 모든 노드 공통 (네트워크 식별)
- **Device Key**: 노드별 고유 키 (Device ID 기반 파생)

```c
// 키 파생 예시
DeviceKey = AES-128(MasterKey, DeviceID || "REVITA")
```

---

## 2. 권장 에러 Correction: LoRa 내장 FEC (CR 4/6) + CRC-16

### 2.1 선택 이유

| 장점 | 설명 |
|------|------|
| **단일 홉 최적화** | Star topology는 단일 홉으로 복잡한 FEC 불필요 |
| **내장 FEC 활용** | SX1262의 하드웨어 FEC 사용으로 CPU 부담 없음 |
| **균형잡힌 CR** | CR 4/6은 에러 복구와 전송 시간의 균형점 |
| **이중 검증** | LoRa CRC + 애플리케이션 CRC로 신뢰성 향상 |

### 2.2 LoRa Coding Rate 비교

| Coding Rate | 오버헤드 | 에러 복구 능력 | 전송 시간 | 권장 환경 |
|-------------|----------|---------------|----------|----------|
| CR 4/5 | 25% | 낮음 | 빠름 | 간섭 적은 환경 |
| **CR 4/6** | 50% | **중간** | **중간** | **일반적 환경 (권장)** |
| CR 4/7 | 75% | 높음 | 느림 | 간섭 많은 환경 |
| CR 4/8 | 100% | 매우 높음 | 매우 느림 | 극한 환경 |

### 2.3 Coding Rate별 전송 시간 상세 비교 (SF9, BW125, 27바이트)

| Coding Rate | 오버헤드 | Payload 전송 시간 | 총 전송 시간 | CR 4/5 대비 |
|-------------|----------|------------------|-------------|-------------|
| **CR 4/5** | 25% | ~51 ms | **~67 ms** | 기준 |
| CR 4/6 | 50% | ~61 ms | ~77 ms | +15% |
| CR 4/7 | 75% | ~71 ms | ~87 ms | +30% |
| **CR 4/8** | 100% | ~81 ms | **~97 ms** | +45% |

#### 계산 원리

```
CR 비율 = (4 + parity bits) / 4

CR 4/5: 5/4 = 1.25 (25% 오버헤드)
CR 4/8: 8/4 = 2.00 (100% 오버헤드)

시간 비율 = CR 4/8 / CR 4/5 = 8/5 = 1.6배
```

#### CR 4/5 vs CR 4/8 트레이드오프

| 비교 항목 | CR 4/5 | CR 4/8 | 차이 |
|----------|--------|--------|------|
| 전송 시간 | 67 ms | 97 ms | **+30 ms (+45%)** |
| 에러 복구 능력 | 낮음 | 매우 높음 | **2배 향상** |
| 배터리 소모 | 기준 | +45% | 전송 시간 비례 |
| 권장 환경 | 간섭 적음 | 극한 환경 | - |

**결론**: CR 4/8은 에러 복구 능력이 2배 좋지만, 전송 시간이 45% 증가하고 배터리 소모도 비례하여 증가합니다. 일반적인 환경에서는 **CR 4/5 또는 CR 4/6**이 적합합니다.

### 2.4 에러 검출/복구 계층

```
┌────────────────────────────────────────────────┐
│              에러 처리 계층 구조                 │
├────────────────────────────────────────────────┤
│ Layer 4: Application CRC-16 (선택적)           │
│          - 암호화 전 payload에 추가             │
│          - 복호화 후 무결성 최종 확인           │
├────────────────────────────────────────────────┤
│ Layer 3: CCM MAC Tag (4-8 bytes)               │
│          - 암호화 데이터 인증                   │
│          - 변조 감지                            │
├────────────────────────────────────────────────┤
│ Layer 2: LoRa CRC-16 (하드웨어)                │
│          - 패킷 레벨 에러 검출                  │
│          - 자동 처리                            │
├────────────────────────────────────────────────┤
│ Layer 1: LoRa FEC CR 4/6 (하드웨어)            │
│          - Forward Error Correction            │
│          - 비트 에러 자동 복구                  │
└────────────────────────────────────────────────┘
```

### 2.5 추가 에러 처리가 필요 없는 이유

1. **Star Topology 특성**
   - 단일 홉 통신으로 다중 홉의 에러 누적 없음
   - 중앙 허브가 모든 통신 관리

2. **LoRa 물리 계층의 강건함**
   - Chirp Spread Spectrum으로 잡음 내성 높음
   - 내장 FEC로 대부분의 비트 에러 복구

3. **16바이트 짧은 Payload**
   - 짧은 패킷은 에러 발생 확률 낮음
   - Reed-Solomon 같은 복잡한 FEC는 오버헤드 대비 효과 적음

---

## 3. 최종 패킷 구조

### 3.1 전송 패킷 (총 27바이트)

```
┌────────────────────────────────────────────────────────┐
│                   LoRa 패킷 (27 bytes)                 │
├──────────┬───────────┬──────────────────┬─────────────┤
│ Header   │  Nonce    │  Encrypted Data  │  MAC Tag    │
│ (2 bytes)│ (5 bytes) │   (16 bytes)     │ (4 bytes)   │
└──────────┴───────────┴──────────────────┴─────────────┘
```

### 3.2 헤더 구조 (2바이트)

| 비트 | 필드 | 설명 |
|------|------|------|
| 15-12 | Version | 프로토콜 버전 (0-15) |
| 11-8 | Type | 메시지 타입 (Data/Ack/Command) |
| 7-0 | Device ID (LSB) | 장치 식별자 하위 8비트 |

### 3.3 Nonce 구조 (5바이트, 축약형)

| 바이트 | 필드 | 설명 |
|--------|------|------|
| 0-1 | Device ID | 장치 식별자 상위 16비트 |
| 2-4 | Frame Counter | 24비트 프레임 카운터 |

**참고**: 전체 13바이트 Nonce는 수신측에서 헤더 정보와 조합하여 재구성

### 3.4 Payload 구조 (16바이트, 암호화 전)

```c
typedef struct {
    uint8_t  msg_type;      // 1 byte: 메시지 타입
    uint8_t  status;        // 1 byte: 상태 플래그
    uint16_t sensor_data[6]; // 12 bytes: 센서 데이터
    uint16_t crc16;         // 2 bytes: 애플리케이션 CRC
} __packed payload_t;       // Total: 16 bytes
```

### 3.5 패킷 계층 구조 (중요)

27바이트 패킷에 포함된 것과 LoRa 물리계층에서 처리되는 것을 구분해야 합니다.

#### 애플리케이션 레벨 패킷 (27바이트)에 포함된 것

| 항목 | 포함 여부 | 크기 | 설명 |
|------|----------|------|------|
| **Header** | ✅ 포함 | 2 bytes | 버전, 타입, Device ID |
| **Nonce** | ✅ 포함 | 5 bytes | Device ID + Frame Counter |
| **암호화 데이터 (AES-CCM)** | ✅ 포함 | 16 bytes | 암호화된 Payload |
| **메시지 인증 (MAC)** | ✅ 포함 | 4 bytes | CCM MAC Tag |
| **Error Correction (FEC)** | ❌ 미포함 | - | LoRa 물리계층에서 자동 처리 |
| **CRC** | ❌ 미포함 | - | LoRa 물리계층에서 자동 추가 |

#### 실제 무선 전송 구조

```
┌─────────────────────────────────────────────────────────────────┐
│           애플리케이션 레벨 (개발자가 구현, 27 bytes)             │
│     Header(2) + Nonce(5) + Encrypted(16) + MAC(4)               │
└─────────────────────────────────────────────────────────────────┘
                              ↓
                     LoRa 물리계층 처리 (SX1262 자동)
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│              실제 무선 전송 (Air Interface)                      │
├─────────────────────────────────────────────────────────────────┤
│  Preamble │ LoRa Header │ 27 bytes │ FEC bits │ CRC-16         │
│  (8 sym)  │  (auto)     │ (data)   │ (CR 4/6) │ (auto)         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ├── FEC: CR 4/6 = 50% 오버헤드
                              │        27 bytes → ~40.5 bytes (coded)
                              │
                              └── 총 Air Time: ~77 ms (SF9, BW125)
```

#### 계층별 역할 분담

| 계층 | 처리 내용 | 개발자 작업 |
|------|----------|------------|
| **애플리케이션** | AES-CCM 암호화, MAC 생성, 패킷 조립 | 직접 구현 필요 |
| **LoRa 드라이버** | FEC 인코딩, CRC 추가, 변조 | Coding Rate 설정만 |
| **SX1262 HW** | RF 전송, Preamble 생성 | 자동 처리 |

```c
// 개발자는 LoRa 설정에서 CR만 지정하면 FEC 자동 적용
struct lora_modem_config config = {
    .coding_rate = CR_4_6,  // 이 설정만으로 50% FEC 오버헤드 자동 적용
    // ...
};
```

---

## 4. Zephyr 구현 가이드

### 4.1 필요한 Kconfig 설정

```kconfig
# prj.conf

# Crypto
CONFIG_TINYCRYPT=y
CONFIG_TINYCRYPT_AES=y
CONFIG_TINYCRYPT_AES_CCM=y

# 또는 mbedTLS 사용 시
# CONFIG_MBEDTLS=y
# CONFIG_MBEDTLS_CIPHER_CCM_ENABLED=y

# LoRa
CONFIG_LORA=y
CONFIG_LORA_SX12XX=y
CONFIG_SPI=y
```

### 4.2 LoRa 설정 예시

```c
// LoRa 파라미터 설정
struct lora_modem_config config = {
    .frequency = 920900000,      // 920.9 MHz (KR 대역)
    .bandwidth = BW_125_KHZ,
    .datarate = SF_9,            // SF9: 균형잡힌 거리/속도
    .coding_rate = CR_4_6,       // 권장 Coding Rate
    .preamble_len = 8,
    .tx_power = 14,              // 14 dBm
    .iq_inverted = false,
    .public_network = false,     // Private network
};
```

### 4.3 AES-CCM 암호화 예시

```c
#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/constants.h>

#define KEY_SIZE    16
#define NONCE_SIZE  13
#define MAC_SIZE    4
#define DATA_SIZE   16

static uint8_t device_key[KEY_SIZE];
static uint32_t frame_counter = 0;

int encrypt_payload(const uint8_t *plaintext, uint8_t *ciphertext,
                    uint8_t *mac, uint16_t device_id)
{
    struct tc_ccm_mode_struct ccm;
    struct tc_aes_key_sched_struct sched;
    uint8_t nonce[NONCE_SIZE];

    // Nonce 구성
    memset(nonce, 0, NONCE_SIZE);
    nonce[0] = (device_id >> 8) & 0xFF;
    nonce[1] = device_id & 0xFF;
    nonce[2] = (frame_counter >> 16) & 0xFF;
    nonce[3] = (frame_counter >> 8) & 0xFF;
    nonce[4] = frame_counter & 0xFF;
    nonce[5] = 0x00;  // Direction: uplink

    // AES 키 스케줄 설정
    tc_aes128_set_encrypt_key(&sched, device_key);

    // CCM 초기화
    tc_ccm_config(&ccm, &sched, nonce, NONCE_SIZE, MAC_SIZE);

    // 암호화 + MAC 생성
    tc_ccm_generation_encryption(ciphertext, DATA_SIZE + MAC_SIZE,
                                  NULL, 0,  // Associated data 없음
                                  plaintext, DATA_SIZE,
                                  &ccm);

    // MAC 복사
    memcpy(mac, ciphertext + DATA_SIZE, MAC_SIZE);

    frame_counter++;
    return 0;
}
```

---

## 5. 보안 고려사항

### 5.1 Replay Attack 방지

```c
// 수신측에서 frame counter 검증
static uint32_t last_frame_counter[MAX_DEVICES];

bool validate_frame_counter(uint16_t device_id, uint32_t rx_counter)
{
    if (rx_counter <= last_frame_counter[device_id]) {
        // Replay attack 감지
        return false;
    }
    last_frame_counter[device_id] = rx_counter;
    return true;
}
```

### 5.2 키 저장

```c
// nRF52840 UICR에 키 저장 (보안 영역)
#define KEY_STORAGE_ADDR  (NRF_UICR_BASE + 0x80)

void store_device_key(const uint8_t *key)
{
    // Flash에 키 저장 (한번만 쓰기 가능)
    nrfx_nvmc_bytes_write(KEY_STORAGE_ADDR, key, KEY_SIZE);
}
```

---

## 6. 성능 예측

### 6.1 전송 시간 (SF9, BW125, CR 4/6)

| 항목 | 값 |
|------|-----|
| Preamble | 8 symbols = 4.1 ms |
| Header | 12.3 ms |
| Payload (27 bytes) | ~60 ms |
| **총 전송 시간** | **~77 ms** |

### 6.2 배터리 수명 예측

| 조건 | 값 |
|------|-----|
| 전송 전류 | ~30 mA |
| Sleep 전류 | ~2 uA |
| 1회 전송 에너지 | ~2.3 mJ |
| 1000mAh 배터리 | ~10분 간격 전송 시 2년+ |

---

## 7. 요약

| 항목 | 권장 | 이유 |
|------|------|------|
| **암호화** | AES-128-CCM | HW 가속, 인증+암호화 통합, LoRaWAN 표준 |
| **MAC 크기** | 4 bytes | 16바이트 payload에 적합한 오버헤드 |
| **FEC** | LoRa CR 4/6 | 에러복구와 전송시간 균형 |
| **CRC** | LoRa CRC + App CRC-16 | 이중 검증으로 신뢰성 확보 |

---

*작성일: 2026-03-17*
*작성: Claude Code*
