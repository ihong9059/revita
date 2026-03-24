# RAK4630-9-SM-I Pin Map - REVITA_LINK_v1

## 회로도 정보
- **Board**: main_board
- **Page**: WHOLE_PART
- **Version**: V1.0
- **Created**: 2026-02-01
- **Updated**: 2026-03-18

---

## U1: RAK4630-9-SM-I 핀 배치표

| Pin # | Port Name | 접속 Label | 용도/설명 |
|-------|-----------|------------|-----------|
| 1 | VBUS | - | USB 전원 (미사용) |
| 2 | USB- | - | USB 데이터- (미사용) |
| 3 | USB+ | - | USB 데이터+ (미사용) |
| 4 | P0.13/I2C_SDA | X_EN_B | X축 모터 Enable B |
| 5 | P0.14/I2C_SCL | X_EN_A | X축 모터 Enable A |
| 6 | P0.15/UART2_RX | UART2_RX | UART2 수신 |
| 7 | P0.16/UART2_TX | UART2_TX | UART2 송신 |
| 8 | P0.17/UART2_DE | 12V_EN | 12V 스텝업 Enable |
| 9 | P0.19/UART1_RX | UART1_RX | RS485 UART1 수신 |
| 10 | P0.20/UART1_TX | UART1_TX | RS485 UART1 송신 |
| 11 | P0.21/UART1_DE | VIB_SENSE | 진동 센서 입력 |
| 12 | P0.10/NFC2 | DIO_X | X축 디지털 입력 |
| 13 | P0.09/NFC1 | DIO_Y | Y축 디지털 입력 |
| 14 | GND | GND | 접지 |
| 15 | RF_BT | - | Bluetooth RF (내부) |
| 16 | GND | GND | 접지 |
| 17 | NRF_RESET | RST# | 리셋 신호 |
| 18 | SDCLK | SWCLK | ST-Link 디버그 클럭 |
| 19 | SDIO | SWDIO | ST-Link 디버그 데이터 |
| 20 | VBAT_SX | 3.3V | SX126x 배터리 전원 |
| 21 | VBAT_IO_SX | - | SX126x IO 전원 (내부) |
| 22 | GND | GND | 접지 |
| 23 | P0.24/I2C_SDA_2 | BUZZER_EN | 부저 Enable |
| 24 | P0.25/I2C_SCL_2 | Y_EN_A | Y축 모터 Enable A |
| 25 | P1.01/SW1 | Y_EN_B | Y축 모터 Enable B |
| 26 | P1.02/SW2 | Y_EN_P2 | Y축 모터 Enable P2 |
| 27 | P1.03/LED1 | RE# | RS485 Receiver Enable |
| 28 | P1.04/LED2 | DE | RS485 Driver Enable |
| 29 | P0.03/QSPI_CLK | QSPI_SCLK | Flash 메모리 클럭 |
| 30 | P0.02/QSPI_DIO3 | QSPI_DIO3 | Flash 메모리 DIO3 |
| 31 | P0.28/QSPI_DIO2 | QSPI_DIO2 | Flash 메모리 DIO2 |
| 32 | P0.29/QSPI_DIO1 | QSPI_DIO1 | Flash 메모리 DIO1 |
| 33 | P0.30/QSPI_DIO0 | QSPI_DIO0 | Flash 메모리 DIO0 |
| 34 | P0.26/QSPI_CS | QSPI_CS | Flash 메모리 Chip Select |
| 35 | GND | GND | 접지 |
| 36 | GND | GND | 접지 |
| 37 | RF_LORA | - | LoRa RF (내부) |
| 38 | GND | GND | 접지 |
| 39 | P0.31/AIN7 | BAT_AIN | 배터리 전압 ADC 입력 |
| 40 | P0.05/AIN3 | BTN | 버튼 입력 |
| 41 | P0.04/AIN2 | X_EN_P2 | X축 모터 Enable P2 |
| 42 | GND | GND | 접지 |
| 43 | VDD_NRF | 3.3V | nRF 전원 (3.3V) |
| 44 | VBAT_NRF | 3.3V | nRF 배터리 전원 (3.3V) |

---

## 기능별 핀 그룹

### UART 통신
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| UART1 RX | 9 | P0.19 | UART1_RX |
| UART1 TX | 10 | P0.20 | UART1_TX |
| UART2 RX | 6 | P0.15 | UART2_RX |
| UART2 TX | 7 | P0.16 | UART2_TX |

### X축 모터 제어 (3-Line-motor-control)
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| Enable A | 5 | P0.14 | X_EN_A |
| Enable B | 4 | P0.13 | X_EN_B |
| Enable P2 | 41 | P0.04 | X_EN_P2 |

### Y축 모터 제어 (3-Line-motor-control_1)
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| Enable A | 24 | P0.25 | Y_EN_A |
| Enable B | 25 | P1.01 | Y_EN_B |
| Enable P2 | 26 | P1.02 | Y_EN_P2 |

### 디지털 입력 (12V → 3.3V 레벨 변환)
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| X축 DIO | 12 | P0.10 | DIO_X |
| Y축 DIO | 13 | P0.09 | DIO_Y |

### QSPI Flash 메모리 (MX25R1635FZUIL0)
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| SCLK | 29 | P0.03 | QSPI_SCLK |
| DIO0 | 33 | P0.30 | QSPI_DIO0 |
| DIO1 | 32 | P0.29 | QSPI_DIO1 |
| DIO2 | 31 | P0.28 | QSPI_DIO2 |
| DIO3 | 30 | P0.02 | QSPI_DIO3 |
| CS | 34 | P0.26 | QSPI_CS |

### RS485 통신
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| RX | 9 | P0.19 | UART1_RX |
| TX | 10 | P0.20 | UART1_TX |
| DE | 28 | P1.04 | DE |
| RE# | 27 | P1.03 | RE# |

### 디버그 (ST-Link)
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| SWCLK | 18 | SDCLK | SWCLK |
| SWDIO | 19 | SDIO | SWDIO |
| RST# | 17 | NRF_RESET | RST# |

### GPIO / 기타
| 기능 | Pin # | Port | Label |
|------|-------|------|-------|
| 배터리 전압 | 39 | P0.31/AIN7 | BAT_AIN |
| 12V Enable | 8 | P0.17 | 12V_EN |
| 부저 Enable | 23 | P0.24 | BUZZER_EN |
| 진동 센서 | 11 | P0.21 | VIB_SENSE |
| 버튼 입력 | 40 | P0.05 | BTN |

### 전원
| 기능 | Pin # | Port | 전압 |
|------|-------|------|------|
| VDD_NRF | 43 | VDD_NRF | 3.3V |
| VBAT_NRF | 44 | VBAT_NRF | 3.3V |
| VBAT_SX | 20 | VBAT_SX | 3.3V |
| GND | 14, 16, 22, 35, 36, 38, 42 | GND | 0V |

---

## 주요 IC

| Reference | Part Number | 용도 |
|-----------|-------------|------|
| U1 | RAK4630-9-SM-I | LoRa + BLE MCU 모듈 |
| U15 | MX25R1635FZUIL0 | 16Mbit QSPI Flash |
| L2 | BLM18PG121SN1D | 페라이트 비드 |

---

*Generated from REVITA_LINK_v1 Schematic (2026-03-24)*
