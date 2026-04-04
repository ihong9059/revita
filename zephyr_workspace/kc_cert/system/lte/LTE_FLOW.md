# RC76xx LTE Module Control Flow

Sierra Wireless AirPrime RC76xx AT Command 기반 제어 흐름

---

## RC76xx SKU (모델) 비교

SKU(Stock Keeping Unit) = 같은 시리즈 내 변종별 옵션 구성.
**동일 모델명이라도 주문 옵션(SKU)에 따라 GPS 탑재 여부가 다름.**

### 모델별 기능 비교

| 모델 | 네트워크 | LTE Cat | 음성 | GNSS | 비고 |
|------|---------|---------|------|------|------|
| **RC7611** | LTE Only | Cat4 | VoLTE | SKU별 | |
| RC7611-1 | LTE Only | Cat1 | VoLTE | SKU별 | |
| RC7612 | LTE + UMTS | Cat4 | Data Only | SKU별 | Limited Availability |
| RC7612-1 | LTE + UMTS | Cat1 | Data Only | SKU별 | Limited Availability |
| **RC7620** | LTE + UMTS + GSM | Cat4 | CS + VoLTE | SKU별 | |
| RC7620-1 | LTE + UMTS + GSM | Cat1 | CS + VoLTE | SKU별 | |
| **RC7630** | LTE Only | Cat4 | VoLTE | SKU별 | 일본 밴드 |
| RC7630-1 | LTE Only | Cat1 | VoLTE | SKU별 | 일본 밴드 |

### GNSS 지원 현황 (SKU-dependent)

| 기능 | RC7612 | RC7612-1 | RC7630 | RC7630-1 | RC7620 | RC7620-1 | RC7611 | RC7611-1 |
|------|--------|----------|--------|----------|--------|----------|--------|----------|
| GPS | Y* | | Y* | Y* | Y* | | Y* | |
| GLONASS | | | | | | | | |
| Galileo | | | | | | | | |
| BeiDou | | | | | | | | |
| QZSS | | | Y | Y | | | | |

*Y\* = SKU-dependent (주문 옵션에 따라 포함/미포함)*

### 한국 LTE 밴드 지원

| 통신사 | 주요 밴드 | RC7611 | RC7620 | RC7630 |
|--------|---------|--------|--------|--------|
| KT | B1, B3, B8 | - | B1,B3,B8 | B1,B3,B5,B8 |
| SKT | B1, B3, B5 | - | B1,B3 | B1,B3,B5 |
| LGU+ | B1, B3, B5, B7 | - | B1,B3,B7 | B1,B3,B5,B7 |

**참고**: RC7611은 B1,B3 미지원 (북미 밴드 전용). **한국에서는 RC7620 또는 RC7630 권장**.

### SKU 확인 AT 명령

```
ATI                → 모델명 (RC7611, RC7620 등)
AT!SKU             → 정확한 SKU 번호
AT+GMR             → 펌웨어 버전
AT+KGSN            → IMEI + Serial Number
AT!GNSSCONFIG?     → GNSS 활성화 상태 (GPS 지원 시)
```

---

## GNSS (GPS) 제어 흐름

GNSS 엔진: QUALCOMM iZat Gen8C (GPS L1, Galileo E1, BeiDou B1, GLONASS L1)

### GNSS 성능 사양

| 항목 | 값 |
|------|-----|
| 추적 감도 | -160 dBm |
| Cold Start 감도 | -145 dBm |
| MS-Assisted 감도 | -158 dBm |
| 정확도 (Open Sky) | < 2m CEP-50 |
| TTFF - Hot Start | 1초 |
| TTFF - Warm Start | 29초 |
| TTFF - Cold Start | 32초 |
| 최대 고도 | 18,288m |
| 최대 속도 | 1,852 km/h |
| 위성 채널 (취득) | 118 |
| 위성 채널 (추적) | 40 |

### GNSS 제어 흐름

```
[LTE 모듈 부팅 완료]
    │
    ▼
┌───────────────────────┐
│ G1. GNSS 지원 확인      │
│   AT!GNSSCONFIG?       │
│   → GPS/GLONASS/BDS/GAL│
│     활성화 상태 확인      │
└──────────┬────────────┘
           │ 미지원 시 → GPSENABLE 확인
           ▼
┌───────────────────────┐
│ G2. Constellation 설정  │
│   AT!GNSSCONFIG=       │
│     1,1,1,1            │
│   (GPS+GLO+BDS+GAL)   │
│   ※ 리셋 후 적용        │
└──────────┬────────────┘
           │
           ▼
┌───────────────────────┐
│ G3. NMEA 출력 설정      │
│   AT!GPSNMEACONFIG=1,1 │
│   (NMEA 활성, 1초 간격) │
│                        │
│   AT!GPSNMEASENTENCE=  │
│     0x03 (GGA+RMC)    │
└──────────┬────────────┘
           │
           ▼
┌───────────────────────┐
│ G4. 측위 시작           │
│   [1회 측위]            │
│   AT!GPSFIX=1,30,100  │
│   (Standalone,30s,100m)│
│                        │
│   [연속 추적]           │
│   AT!GPSTRACK=1,30,100,│
│     1000,10            │
│   (1000회, 10초 간격)   │
└──────────┬────────────┘
           │
           ▼
┌───────────────────────┐
│ G5. 상태/결과 확인       │
│   AT!GPSSTATUS?        │
│   → ACTIVE / SUCCESS   │
│                        │
│   AT!GPSLOC?           │
│   → Lat, Lon, Alt,     │
│     Time, HEPE, Fix    │
└──────────┬────────────┘
           │
           ▼
┌───────────────────────┐
│ G6. 위성 정보 확인       │
│   AT!GPSSATINFO?       │
│   → SV, ELEV, AZI, SNR│
└──────────┬────────────┘
           │
           ▼
┌───────────────────────┐
│ G7. 세션 종료           │
│   AT!GPSEND=0          │
└───────────────────────┘
```

### GNSS AT 명령어 상세

#### G1. GNSS Constellation 설정
```
AT!GNSSCONFIG=<GPS>,<GLO>,<BDS>,<GAL>

예시: AT!GNSSCONFIG=1,1,1,1    (GPS + GLONASS + BeiDou + Galileo 전부 활성)
조회: AT!GNSSCONFIG?
응답: GPS:     1
      GLONASS: 1
      BDS:     1
      GAL:     1
      OK

※ GPS는 비활성화 불가 (항상 1)
※ 변경 후 리셋 필요 (persistent across power cycles)
```

#### G2. NMEA 출력 설정
```
AT!GPSNMEACONFIG=<enable>[,<output_rate>]

예시: AT!GPSNMEACONFIG=1,1     (활성화, 1초 간격)
      AT!GPSNMEACONFIG=0       (비활성화)

※ 리셋 후 적용
```

#### G3. NMEA 문장 종류 설정
```
AT!GPSNMEASENTENCE=<nmea_type>

<nmea_type>: 2바이트 hex bitmask
  Bit 0:  GPGGA  (위치+고도+위성수)
  Bit 1:  GPRMC  (위치+시간+속도+날짜)
  Bit 2:  GPGSV  (가시 위성 상세)
  Bit 3:  GPGSA  (위성 전체 DOP)
  Bit 4:  GPVTG  (속도+방위)
  Bit 5:  PQXFI  (Qualcomm 확장)
  Bit 6:  GLGSV  (GLONASS GSV)
  Bit 7:  GNGSA  (GLONASS GSA)
  Bit 8:  GNGNS  (GLONASS 위치)
  Bit 21: GNGGA  (GNSS GGA)
  Bit 22: GNRMC  (GNSS RMC)
  Bit 30: GPGLL  (위도/경도)

예시: AT!GPSNMEASENTENCE=03    → GGA + RMC 활성화
      AT!GPSNMEASENTENCE=0F    → GGA + RMC + GSV + GSA
```

#### G4-a. 1회 측위 (!GPSFIX)
```
AT!GPSFIX=<fixType>,<maxTime>,<maxDist>

<fixType>: 1=Standalone, 2=MS-based, 3=MS-assisted
<maxTime>: 1~255초 (측위 제한 시간)
<maxDist>: 1~4294967279 미터 (요구 정확도), 4294967280=무관

예시: AT!GPSFIX=1,30,100       (Standalone, 30초, 100m)
      AT!GPSFIX=1,60,10        (Standalone, 60초, 10m 정확도)

※ 완료 후 AT!GPSLOC?로 결과 확인
```

#### G4-b. 연속 추적 (!GPSTRACK)
```
AT!GPSTRACK=<fixType>,<maxTime>,<maxDist>,<fixCount>,<fixRate>

<fixCount>: 1~1000 (1000=연속)
<fixRate>:  1~65535초 (fix 간격)

예시: AT!GPSTRACK=1,15,10,20,60
      → Standalone, 15초 제한, 10m, 20회, 60초 간격

      AT!GPSTRACK=1,30,100,1000,10
      → 연속 추적, 10초 간격

※ 첫 fix는 almanac/ephemeris 갱신으로 더 오래 걸릴 수 있음
※ 시작 전 AT!GPSFIX로 단발 fix 1회 수행 권장
```

#### G5. 상태 확인 (!GPSSTATUS)
```
AT!GPSSTATUS?

응답 예시:
  2026 04 05 6 10:25:01 Last Fix Status = SUCCESS
  2026 04 05 6 10:25:02 Fix Session Status = ACTIVE
  OK

<status>: NONE / ACTIVE / SUCCESS / FAIL
```

#### G6. 위치 결과 조회 (!GPSLOC)
```
AT!GPSLOC?

응답 예시:
  Lat: 37 Deg 33 Min 55.20 Sec N (0x...)
  Lon: 126 Deg 58 Min 30.40 Sec E (0x...)
  Time: 2026 04 05 10:25:03 (GPS)
  LocUncAngle: 0.0 deg  LocUncA: 5.0 m  LocUncP: 5.0 m
  HEPE: 7.071 m
  3D Fix
  Altitude: 50 m  LocUncVe: 10.0 m
  Heading: 45.0 deg  VelHoriz: 0.0 m/s  VelVert: 0.0 m/s
  OK

※ "Unknown" → 측위 전
※ "Not Available" → 데이터 없음
```

#### G7. 위성 정보 (!GPSSATINFO)
```
AT!GPSSATINFO?

응답 예시:
  Satellites in view: 12
  * SV: 3   ELEV: 45  AZI: 120  SNR: 35
  * SV: 7   ELEV: 60  AZI: 200  SNR: 40
    SV: 15  ELEV: 10  AZI: 310  SNR: 15
  ...
  OK

* = 측위에 사용된 위성
SV 범위: 1-32=GPS, 65-96=GLONASS, 201-237=BeiDou, 301-336=Galileo
```

#### G8. 세션 종료
```
AT!GPSEND=0
OK
```

### GNSS 자동 시작 설정
```
AT!GPSAUTOSTART=<function>,<fixtype>,<maxtime>,<maxdist>,<fixrate>

<function>: 0=비활성, 1=부팅 시, 2=NMEA 포트 열릴 때

예시: AT!GPSAUTOSTART=1,1,30,100,10
      → 부팅 시 자동으로 GPS Standalone 시작

※ 리셋 후 적용, persistent
```

### GNSS Cold Start (보조 데이터 초기화)
```
AT!GPSCOLDSTART      → 모든 GNSS 보조 데이터 삭제 (다음 fix는 Cold Start)

AT!GPSCLRASSIST=<eph>,<alm>,<pos>,<time>,<iono>
  예: AT!GPSCLRASSIST=1,1,1,1,1  → 전부 삭제 (= COLDSTART)
      AT!GPSCLRASSIST=1,0,0,0,0  → ephemeris만 삭제

※ 비밀번호 필요 (AT!ENTERCND 사용)
```

### GNSS + LTE 동시 사용 참고사항

- GNSS는 LTE와 **동시 동작** 가능 (별도 RF 경로: RF_GNSS pin 38)
- GNSS 안테나와 WWAN 안테나는 **분리** (isolation > 20dB 권장)
- GNSS 비활성 SKU에서 활성화: `AT!CUSTOM="GPSENABLE",1` 후 리셋
- XTRA (보조 데이터): LTE 데이터 연결 시 NDIS로 자동 다운로드 → Cold Start 시간 단축

---

## HW 연결 구성

```
nRF52840 (RAK4631)                    RC76xx LTE Module
┌─────────────┐                      ┌──────────────┐
│  UART1 TX   │─── P0.16 ──────────>│ UART1_TX (5) │
│  (RAK_UART2)│                      │  (Host→Module)│
│  UART1 RX   │<── P0.15 ──────────│ UART1_RX (6) │
│  (RAK_UART2)│                      │  (Module→Host)│
│             │                      │              │
│  I2C SDA    │─── P0.13 ──┐        │              │
│  I2C SCL    │─── P0.14 ──┤        │              │
└─────────────┘            │        │              │
                           │        │ RESET_IN_N   │
                    MCP23017│        │ POWER_ON_N   │
                    (0x20)  │        └──────────────┘
                    ┌───────┤
                    │ GPA0  │──> LTE_RST_N (Active Low)
                    │ GPA3  │──> MUX_SEL
                    │ GPA4  │──> MUX_EN#
                    └───────┘
```

### UART 설정
| 항목 | 값 |
|------|-----|
| 인터페이스 | UART1 (RAK_UART2) |
| MCU TX | P0.16 → RC76xx UART1_TX (pin 5) |
| MCU RX | P0.15 ← RC76xx UART1_RX (pin 6) |
| Baud Rate | 115200 bps |
| Data/Stop/Parity | 8N1 |
| Flow Control | None (HW flow control 미사용) |

### Debug Console
| 항목 | 값 |
|------|-----|
| 인터페이스 | UART0 (RS485 경로) |
| TX | P0.20, RX | P0.19 |
| 용도 | printk 디버그 출력 |

---

## 전체 제어 흐름

```
[전원 인가]
    │
    ▼
┌──────────────────────┐
│ 1. HW 초기화          │
│   - GPIO, I2C, UART  │
│   - MCP23017 검색     │
│   - MUX RS485 활성화   │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 2. LTE 모듈 리셋      │
│   - GPA0 = LOW (500ms)│
│   - GPA0 = HIGH       │
│   - 부팅 대기 (5~7초)  │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 3. AT 통신 확인        │
│   - ATE0 (에코 끄기)   │
│   - AT → OK 확인      │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 4. 모듈 정보 확인      │
│   - ATI (제품 정보)    │
│   - AT+GMR (FW 버전)  │
│   - AT+KGSN (IMEI)   │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 5. SIM 상태 확인       │
│   - AT+CPIN? → READY │
│   - AT+CCID (ICCID)  │
└──────────┬───────────┘
           │ SIM READY?
           │ NO → [에러: SIM 없음]
           ▼ YES
┌──────────────────────┐
│ 6. 네트워크 등록 대기   │
│   - AT+CREG? 반복 폴링 │
│   - 0,1=Home 0,5=Roam│
│   - 최대 60초 대기     │
└──────────┬───────────┘
           │ 등록 완료?
           │ NO → [에러: 네트워크 실패]
           ▼ YES
┌──────────────────────┐
│ 7. 신호/네트워크 정보   │
│   - AT+CSQ (신호 품질) │
│   - AT+COPS? (통신사)  │
│   - AT+CGATT? (PS연결) │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 8. PDP Context 설정   │
│   - AT+CGDCONT       │
│   - AT+KCNXCFG       │
│   - AT+KCNXUP        │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 9. 데이터 통신         │
│   - TCP / UDP / HTTP │
│   (용도에 따라 선택)    │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 10. 주기적 모니터링     │
│   - AT+CSQ (신호)     │
│   - AT+CREG? (등록)   │
│   - 연결 상태 확인     │
└──────────────────────┘
```

---

## Phase 1: HW 초기화

MCU 부팅 후 주변 디바이스 초기화.

```c
/* GPIO: RS485 방향 제어 */
gpio_pin_configure(gpio0, PIN_RS485_DE, GPIO_OUTPUT_LOW);
gpio_pin_configure(gpio0, PIN_RS485_RE_N, GPIO_OUTPUT_LOW);

/* I2C: MCP23017 검색 (0x20~0x27) */
/* UART1: LTE 모듈 통신 (인터럽트 RX) */
```

---

## Phase 2: LTE 모듈 리셋 (MCP23017 GPA0)

MCP23017의 GPA0 핀이 RC76xx의 RESET_IN_N에 연결됨 (Active Low).

```
시퀀스:
1. GPA0 = OUTPUT 설정 (IODIRA bit0 = 0)
2. GPA0 = LOW  → 리셋 활성화
3. 500ms 대기
4. GPA0 = HIGH → 리셋 해제
5. 5~7초 대기  → 모듈 부팅 완료 (t_pwr_on_seq: typ 7s, max 24s)
```

**주의**: POWER_ON_N은 200ms~7s LOW 펄스로 켜짐. RESET_IN_N과 별도 핀.
현재 회로에서는 GPA0 = LTE_RST_N 만 제어 가능.

---

## Phase 3: AT 통신 확인

```
MCU → LTE: ATE0\r\n          (에코 끄기)
LTE → MCU: OK\r\n

MCU → LTE: AT\r\n            (기본 통신 확인)
LTE → MCU: OK\r\n
```

**응답 대기**: 2초 timeout. "OK" 또는 "ERROR" 수신 시 즉시 리턴.
모듈이 아직 부팅 중이면 응답 없음 → 재시도 (최대 10회, 1초 간격).

---

## Phase 4: 모듈 정보 확인

```
MCU → LTE: ATI\r\n
LTE → MCU: Sierra Wireless
           RC7611
           ...
           OK

MCU → LTE: AT+GMR\r\n        (펌웨어 버전)
LTE → MCU: SWI9X07H_00.08.24.00
           OK

MCU → LTE: AT+KGSN\r\n       (IMEI + S/N)
LTE → MCU: +KGSN: 353456789012345,A1B2C3D4
           OK
```

---

## Phase 5: SIM 상태 확인

```
MCU → LTE: AT+CPIN?\r\n
LTE → MCU: +CPIN: READY       ← SIM 정상
           OK

MCU → LTE: AT+CCID\r\n        (SIM ICCID)
LTE → MCU: +CCID: 8982012345678901234
           OK
```

**+CPIN 응답값**:
| 응답 | 의미 |
|------|------|
| READY | SIM 정상, PIN 불필요 |
| SIM PIN | PIN 입력 필요 (AT+CPIN=<pin>) |
| SIM PUK | PUK 필요 |
| (ERROR) | SIM 없거나 인식 실패 |

---

## Phase 6: 네트워크 등록

```
MCU → LTE: AT+CREG?\r\n
LTE → MCU: +CREG: 0,1         ← 홈 네트워크 등록 완료
           OK
```

**+CREG 두 번째 값 (<stat>)**:
| 값 | 상태 |
|----|------|
| 0 | 미등록, 검색 안함 |
| 1 | 홈 네트워크 등록 완료 |
| 2 | 미등록, 검색 중 |
| 3 | 등록 거부 |
| 5 | 로밍 등록 완료 |

**구현**: stat이 1 또는 5가 될 때까지 3초 간격으로 폴링 (최대 60초).

```c
for (int retry = 0; retry < 20; retry++) {
    at_cmd("AT+CREG?", resp, sizeof(resp), 2000);
    if (strstr(resp, ",1") || strstr(resp, ",5")) {
        /* 등록 성공 */
        break;
    }
    k_sleep(K_SECONDS(3));
}
```

---

## Phase 7: 신호/네트워크 정보

### AT+CSQ (신호 품질)
```
MCU → LTE: AT+CSQ\r\n
LTE → MCU: +CSQ: 20,99
           OK
```

| rssi 값 | dBm | 품질 |
|---------|-----|------|
| 0 | -113 이하 | 없음 |
| 1 | -111 | 매우 약함 |
| 2~30 | -109 ~ -53 | rssi*2-113 |
| 31 | -51 이상 | 매우 강함 |
| 99 | 측정 불가 | - |

### AT+COPS? (통신사)
```
MCU → LTE: AT+COPS?\r\n
LTE → MCU: +COPS: 0,0,"KT",7    ← 자동 선택, KT, LTE
           OK
```

### AT+CGATT? (PS 연결)
```
MCU → LTE: AT+CGATT?\r\n
LTE → MCU: +CGATT: 1             ← PS 연결됨
           OK
```

---

## Phase 8: PDP Context (데이터 연결)

LTE 데이터 통신을 위해 PDP Context를 설정하고 활성화.

### Step 8-1: PDP Context 정의
```
AT+CGDCONT=1,"IP","<APN>"
OK
```
**한국 통신사 APN 예시**:
| 통신사 | APN |
|--------|-----|
| KT | lte.ktfwing.com |
| SKT | lte-internet.sktelecom.com |
| LGU+ | internet.lguplus.co.kr |

### Step 8-2: GPRS 연결 설정
```
AT+KCNXCFG=1,"GPRS","<APN>"
OK
```

### Step 8-3: PDP 연결 활성화
```
AT+KCNXUP=1
OK
```

### Step 8-4: IP 주소 확인
```
AT+KCGPADDR=1
+KCGPADDR: 1,10.xxx.xxx.xxx
OK
```

---

## Phase 9: 데이터 통신

PDP 연결 후, 용도에 따라 TCP/UDP/HTTP 중 선택.

### Option A: TCP Client

```
[1] 세션 설정
AT+KTCPCFG=1,0,"<server_ip>",<port>
+KTCPCFG: 1                       ← session_id = 1
OK

[2] 연결
AT+KTCPCNX=1
OK
+KTCP_IND: 1,1                    ← 연결 성공 (URC)

[3] 데이터 송신
AT+KTCPSND=1,<length>
CONNECT                           ← 데이터 모드 진입
<data bytes>                      ← 데이터 전송
--EOF--Pattern--                  ← EOF 패턴 전송
OK

[4] 데이터 수신 (URC)
+KTCP_DATA: 1,<ndata>             ← 수신 데이터 알림
AT+KTCPRCV=1,<ndata>
CONNECT
<received data>
--EOF--Pattern--
OK

[5] 연결 종료
AT+KTCPCLOSE=1,1
OK

[6] 세션 삭제
AT+KTCPDEL=1
OK
```

### Option B: UDP Client

```
[1] 세션 설정
AT+KUDPCFG=1,0,"<server_ip>",<port>
+KUDPCFG: 1
OK

[2] 데이터 송신
AT+KUDPSND=1,<length>
CONNECT
<data bytes>
--EOF--Pattern--
OK

[3] 데이터 수신 (URC)
+KUDP_DATA: 1,<ndata>
AT+KUDPRCV=1,<ndata>
CONNECT
<received data>
--EOF--Pattern--
OK

[4] 종료
AT+KUDPCLOSE=1
OK
```

### Option C: HTTP GET

```
[1] HTTP 세션 설정
AT+KHTTPCFG=1,"<server>",<port>,0
+KHTTPCFG: 1                      ← session_id = 1
OK

[2] HTTP GET 요청
AT+KHTTPGET=1,"/api/data"
CONNECT
HTTP/1.1 200 OK
Content-Type: application/json
...
{"key":"value"}
--EOF--Pattern--
OK

[3] 세션 종료
AT+KHTTPCLOSE=1
OK
```

---

## Phase 10: 주기적 모니터링

네트워크 연결 유지 및 상태 감시.

```c
while (1) {
    /* 네트워크 등록 상태 */
    at_cmd("AT+CREG?", resp, sizeof(resp), 2000);

    /* 신호 품질 */
    at_cmd("AT+CSQ", resp, sizeof(resp), 2000);

    /* 15초 대기 */
    k_sleep(K_SECONDS(15));
}
```

---

## 에러 처리

### AT 응답 코드
| 응답 | 의미 |
|------|------|
| OK | 명령 성공 |
| ERROR | 명령 실패 (파라미터 오류 등) |
| +CME ERROR: <n> | 상세 에러 코드 (AT+CMEE=1로 활성화) |

### 주요 CME 에러 코드
| 코드 | 의미 |
|------|------|
| 10 | SIM 미삽입 |
| 11 | SIM PIN 필요 |
| 13 | SIM 실패 |
| 30 | 네트워크 없음 |

### 복구 시나리오
```
[응답 없음 (timeout)]
  → 재시도 3회 → 모듈 리셋 (GPA0) → 재초기화

[네트워크 등록 실패 (+CREG: 0,3)]
  → AT+CFUN=0 → 2초 대기 → AT+CFUN=1 → 재등록 시도

[PDP 연결 실패]
  → AT+KCNXDOWN=1,1 → 재설정 → AT+KCNXUP=1

[TCP/UDP 세션 에러]
  → AT+KTCPCLOSE / AT+KUDPCLOSE → 세션 재생성
```

---

## AT Command 타이밍

| 구간 | 대기 시간 |
|------|---------|
| 모듈 리셋 후 부팅 | 5~7초 (max 24초) |
| AT 명령 간 간격 | 100ms 이상 |
| +++ 이스케이프 가드 | 전후 1초 |
| 네트워크 등록 대기 | 최대 60초 |
| PDP 활성화 | 최대 30초 |

---

## 전원 관리

### Software Power Down
```
AT!POWERDOWN
OK
```

### Power Saving Mode (PSM)
```
AT+CPSMS=1,,,<TAU>,<Active_Time>
OK
```
- TAU: 주기적 갱신 타이머 (모듈이 깨어나는 주기)
- Active_Time: 깨어있는 시간

---

## 참고 자료

- `zephyr_workspace/lte/RC76xx_AT_Command_Reference_Guide_Rev4.0.pdf`
- `zephyr_workspace/lte/RC76xx_Product_Technical_Specification.pdf`
- AT Command Reference: p.243~ (Protocol Commands - TCP/UDP/HTTP)
- Technical Spec: p.65~ (POWER_ON_N, UART Interface)
