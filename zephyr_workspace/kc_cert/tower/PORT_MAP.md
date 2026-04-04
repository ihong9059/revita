# REVITA TOWER V1 - MCU & UART MUX Port Map

> 회로도: REVITA_TOWER_v1.pdf (EasyEDA, V1.0)
> Created: 2026-02-02 / Updated: 2026-03-01

---

## 1. MCU Part (nRF52840 / RAK4631 Module)

### 1.1 MCU 모듈 커넥터 핀 할당

| nRF52840 Pin | Board Net Name | 기능 | 연결 대상 |
|---|---|---|---|
| P0.13 | I2C_SDA | I2C 데이터 | I2C 버스 (MCP23017, I2C_EXB 등) |
| P0.14 | I2C_SCL | I2C 클럭 | I2C 버스 |
| P0.15 | RAK_UART2_RX | UART2 수신 | LTE 모듈 TX (확인필요) |
| P0.16 | RAK_UART2_TX | UART2 송신 | LTE 모듈 RX (확인필요) |
| P0.17 | RS485_DE | RS485 Driver Enable | RS485 트랜시버 DE (HIGH=송신) |
| P0.19 | RAK_UART1_RX | UART1 수신 | UART_MUX COM_RX |
| P0.20 | RAK_UART1_TX | UART1 송신 | UART_MUX COM_TX |
| P0.21 | RS485_RE# | RS485 Receiver Enable | RS485 트랜시버 RE# (LOW=수신) |
| P0.26 | UARPC2 | GPIO/AIN | 확인필요 |
| P1.01 | SIO_35 | GPIO | 확인필요 |
| P1.02 | SIO_34 | GPIO | 확인필요 |
| - | MCU_USB_D_N | USB D- | USB 커넥터 |
| - | MCU_USB_D_P | USB D+ | USB 커넥터 |
| - | LTE_PW_ON | GPIO 출력 | LTE 모듈 전원 제어 |
| - | LTE_DTR | GPIO 출력 | LTE 모듈 DTR |
| - | RESET | 리셋 | 리셋 회로 |

### 1.2 SPI 핀 (RAK4631 기본 할당)

| nRF52840 Pin | 기능 |
|---|---|
| P0.03 | SPI_CLK |
| P0.26 | SPI_CS |
| P0.29 | SPI_MISO |
| P0.30 | SPI_MOSI |

### 1.3 기타 핀

| nRF52840 Pin | 기능 |
|---|---|
| P0.05 | AIN0 (ADC) |
| P0.31 | AIN1 (ADC) |
| P1.03 | LED1 (Green) |
| P1.04 | LED2 (Blue) |

---

## 2. UART MUX

### 2.1 구조

```
MCU UART1 (P0.20 TX, P0.19 RX)
      │
      ▼
┌─────────────────────────────┐
│   Dual 4:1 Analog MUX      │
│   (74HC4052 또는 동급)       │
│                             │
│   COM_TX ← RAK_UART1_TX    │
│   COM_RX ← RAK_UART1_RX   │
│                             │
│   S0, S1 ← MCP23017 GPA    │
├─────────────────────────────┤
│ CH0 (S1=0,S0=0) → RS485    │
│ CH1 (S1=0,S0=1) → SBC(RPi) │
│ CH2 (S1=1,S0=0) → 확인필요  │
│ CH3 (S1=1,S0=1) → 확인필요  │
└─────────────────────────────┘
```

### 2.2 채널 할당 테이블

| S1 | S0 | 채널 | 목적지 | 비고 |
|---|---|---|---|---|
| 0 | 0 | CH0 | **RS485** | RS485 트랜시버 경유 → 외부 RS485 버스 |
| 0 | 1 | CH1 | **SBC (Raspberry Pi)** | SBC UART 헤더 연결 |
| 1 | 0 | CH2 | LTE / 예비 | 확인필요 |
| 1 | 1 | CH3 | 예비 / NC | 확인필요 |

### 2.3 MUX 셀렉트 제어

- MUX S0, S1은 **MCP23017 I2C I/O 확장기**의 GPA 핀으로 제어 (MCU 직접 GPIO 아님)
- MCP23017은 MCU I2C 버스 (P0.13 SDA, P0.14 SCL)에 연결
- MUX 채널 변경 시 I2C 명령으로 MCP23017 출력 설정 필요

---

## 3. MCP23017 I2C I/O 확장기

### 3.1 연결

| 항목 | 값 |
|---|---|
| I2C 주소 | 0x20 ~ 0x27 (A0-A2 설정에 따라, 확인필요) |
| SDA | P0.13 (MCU I2C_SDA) |
| SCL | P0.14 (MCU I2C_SCL) |

### 3.2 GPIO 출력 할당 (확인필요)

| GPA Pin | 기능 (추정) | GPB Pin | 기능 (추정) |
|---|---|---|---|
| GPA0 | RS485_EN / 제어 | GPB0 | LED / 제어 |
| GPA1 | 제어 | GPB1 | BUZZER_EN |
| GPA2 | MUX_S0 | GPB2 | 제어 |
| GPA3 | MUX_S1 | GPB3 | 제어 |
| GPA4 | LED 제어 | GPB4 | 제어 |
| GPA5 | 제어 | GPB5 | RS_4B5_ROTA (?) |
| GPA6 | 제어 | GPB6 | 제어 |
| GPA7 | 제어 | GPB7 | 제어 |

> **주의**: MCP23017 GPIO 할당은 회로도 해상도 한계로 정확한 판독이 어려움. 실물 보드에서 반드시 확인 필요.

---

## 4. RS485 Port

### 4.1 트랜시버 연결

```
                    RS485 트랜시버 (MAX485/SP3485)
                    ┌──────────┐
RS485_TX (MUX CH0)──┤ DI    A ├──── RS485_A (터미널)
RS485_RX (MUX CH0)──┤ RO    B ├──── RS485_B (터미널)
P0.17 (MCU GPIO)────┤ DE  GND ├──── GND
P0.21 (MCU GPIO)────┤ RE# VCC ├──── 3.3V
                    └──────────┘
```

### 4.2 RS485 방향 제어

| DE (P0.17) | RE# (P0.21) | 모드 |
|---|---|---|
| HIGH | HIGH | **송신 전용** (현재 펌웨어 설정) |
| LOW | LOW | 수신 전용 |
| HIGH | LOW | 송수신 (half-duplex 전환) |
| LOW | HIGH | 비활성 (고임피던스) |

---

## 5. UART 통신 경로 요약

### 5.1 RS485 통신 경로

```
MCU P0.20 (TX) → UART_MUX COM_TX → CH0 → RS485_TX → MAX485 DI → A/B 버스
MCU P0.19 (RX) ← UART_MUX COM_RX ← CH0 ← RS485_RX ← MAX485 RO ← A/B 버스

방향 제어: P0.17 (DE), P0.21 (RE#) → MCU 직접 GPIO
MUX 채널: MCP23017 → S0=0, S1=0 (CH0)
```

### 5.2 SBC (Raspberry Pi) 통신 경로

```
MCU P0.20 (TX) → UART_MUX COM_TX → CH1 → SBC_RX (RPi UART 수신)
MCU P0.19 (RX) ← UART_MUX COM_RX ← CH1 ← SBC_TX (RPi UART 송신)

MUX 채널: MCP23017 → S0=1, S1=0 (CH1)
```

### 5.3 LTE 모듈 통신 경로 (확인필요)

```
옵션 A: UART2 직접 연결 (별도 UART)
  MCU P0.16 (UART2_TX) → LTE_RX
  MCU P0.15 (UART2_RX) ← LTE_TX

옵션 B: UART_MUX CH2 경유
  MCU P0.20 (TX) → UART_MUX → CH2 → LTE_RX
  MCU P0.19 (RX) ← UART_MUX ← CH2 ← LTE_TX
```

---

## 6. 현재 펌웨어 상태 (tower v1.1)

| 항목 | 상태 | 비고 |
|---|---|---|
| UART1 (P0.20/P0.19) | ✅ 활성 | printk 콘솔 출력 (115200 baud) |
| P0.17 (RS485_DE) | ✅ HIGH | 송신 활성 |
| P0.21 (RS485_RE#) | ✅ HIGH | 수신 비활성 |
| UART_MUX 채널 선택 | ❌ 미구현 | MCP23017 I2C 제어 필요 |
| MCP23017 초기화 | ❌ 미구현 | I2C 드라이버 설정 필요 |
| RS485 수신 모드 | ❌ 미구현 | RE#=LOW 전환 로직 필요 |

### 6.1 RS485 출력 확인을 위한 조건

1. **MUX가 CH0 (RS485)로 설정되어야 함**
   - MCP23017 S0=0, S1=0 필요
   - 또는 MUX S0/S1에 풀다운 저항이 있으면 기본값이 CH0 (확인필요)
2. RS485_DE = HIGH (P0.17, 현재 설정됨)
3. UART1 TX에서 데이터 출력 (현재 printk로 출력 중)

> **참고**: MUX S0/S1 라인에 풀다운 저항이 없거나 플로팅 상태이면,
> 전원 인가 시 MUX 채널이 불확정 상태가 되어 RS485 출력이 안 될 수 있음.

---

## 7. 보드 블록 다이어그램

```
┌─────────────────────────────────────────────────────────┐
│                   REVITA TOWER V1                       │
│                                                         │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────┐  │
│  │ POWER    │    │ SOLAR    │    │ Single Board     │  │
│  │ PORT     │───▶│ CHARGER  │───▶│ Computer (RPi)   │  │
│  │ 12~14V   │    │ V6       │    │  - ETH_PORT      │  │
│  └──────────┘    └──────────┘    │  - SBC UART      │  │
│                                   └───────┬──────────┘  │
│                                           │ SBC_TX/RX   │
│  ┌──────────┐                    ┌────────┴─────────┐  │
│  │ ANTI     │    ┌──────────┐    │   UART_MUX       │  │
│  │ THIEF    │    │ MCU_PART │    │   (74HC4052)     │  │
│  └──────────┘    │ RAK4631  │    │                  │  │
│                  │ nRF52840 ├────┤ CH0→RS485        │  │
│  ┌──────────┐    │          │    │ CH1→SBC          │  │
│  │ EXTERNAL │    │ UART1────┼───▶│ CH2→(확인필요)    │  │
│  │ BUTTON   │◀──▶│ I2C──────┼───▶│ CH3→(확인필요)    │  │
│  │ (I2C_EXB)│    │ UART2────┼──┐ └────────┬─────────┘  │
│  └──────────┘    │ GPIO─────┼┐ │          │ RS485_TX/RX │
│                  └──────────┘│ │ ┌────────┴─────────┐  │
│                    P0.17,P0.21│ │ │   RS485_PORT     │  │
│                              │ │ │   (MAX485)       │  │
│  ┌──────────┐                │ │ │   A/B 터미널      │  │
│  │ MCP23017 │◀── I2C ───────┘ │ └──────────────────┘  │
│  │ I/O EXP  │───▶ MUX S0/S1   │                       │
│  │          │───▶ LED, BUZZER  │ ┌──────────────────┐  │
│  └──────────┘                  └▶│   LTE_MODULE     │  │
│                                  │   (UART2 직결?)   │  │
│  ┌──────────┐    ┌──────────┐   └──────────────────┘  │
│  │ POWER    │    │ FIRMWARE │                          │
│  │ SWITCH   │    │ PORT     │ (SWD Debug)              │
│  └──────────┘    └──────────┘                          │
└─────────────────────────────────────────────────────────┘
```

---

*회로도 출처: REVITA_TOWER_v1.pdf*
*작성: Claude Code, 2026-04-04*
*주의: "확인필요" 표시 항목은 회로도 해상도 한계로 실물 보드에서 검증 필요*
