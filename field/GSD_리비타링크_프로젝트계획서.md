# GSD 프로젝트 계획서: 리비타 링크 펌웨어

> **프로젝트명**: REVITA_LINK_FW
> **개발 도구**: Zephyr RTOS
> **타겟 MCU**: RAK4630 (nRF52840 + SX1262)
> **작성일**: 2026-03-24

---

## 1. 프로젝트 개요

### 1.1 제품 설명
리비타 링크는 노지 스마트팜용 센서/제어 단말기입니다.
- LoRa로 리비타 타워와 통신
- RS485 센서 데이터 수집
- 3-Line 모터로 밸브 제어
- 유량계 펄스 카운팅
- 저전력 운영 (태양광 + 배터리)

### 1.2 핵심 목표
- 초저전력 동작 (deep/light sleep)
- 안정적인 LoRa 통신
- 동적 config 기반 운영
- Dual-bank OTA 지원

---

## 2. 개발 환경 설정

### 2.1 필수 도구
```
- Zephyr SDK 0.16.x
- West (Zephyr meta-tool)
- nRF Connect SDK (참고용)
- J-Link / ST-Link 디버거
- VS Code + Zephyr 확장
```

### 2.2 보드 설정
```
Board: rak4631_nrf52840 (커스텀 DTS 필요)
SoC: nRF52840
Flash: 1MB (내장) + 2MB (외장 MX25R1635F)
RAM: 256KB
```

---

## 3. 개발 단계 (Milestone)

### Phase 1: 환경 구축 및 기본 BSP
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 1.1 | Zephyr 개발 환경 설정 | 빌드 환경 |
| 1.2 | RAK4630 커스텀 보드 DTS 작성 | `rak4630_revita_link.dts` |
| 1.3 | 핀맵 정의 (devicetree overlay) | `revita_link.overlay` |
| 1.4 | 기본 빌드 및 플래싱 테스트 | Hello World 동작 확인 |

### Phase 2: HAL 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 2.1 | GPIO 드라이버 (LED, 버튼, 12V_EN) | `hal_gpio.c/h` |
| 2.2 | UART 드라이버 (UART1, UART2) | `hal_uart.c/h` |
| 2.3 | QSPI Flash 드라이버 (MX25R1635F) | `hal_flash.c/h` |
| 2.4 | ADC 드라이버 (배터리 전압) | `hal_adc.c/h` |
| 2.5 | PWM/GPIO 드라이버 (부저) | `hal_buzzer.c/h` |

### Phase 3: 통신 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 3.1 | LoRa 드라이버 (SX1262) | `drv_lora.c/h` |
| 3.2 | LoRa Mesh 통합 (MeshCore 포팅) | `lora_mesh.c/h` |
| 3.3 | BLE 드라이버 (Nordic SoftDevice) | `drv_ble.c/h` |
| 3.4 | RS485 드라이버 (UART + DE/RE#) | `drv_rs485.c/h` |
| 3.5 | MODBUS RTU 프로토콜 | `modbus_rtu.c/h` |

### Phase 4: 주변장치 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 4.1 | 3-Line 모터 제어 (X/Y축 밸브) | `drv_motor_valve.c/h` |
| 4.2 | 유량계 드라이버 (PPI 카운팅) | `drv_flowmeter.c/h` |
| 4.3 | 진동 센서 드라이버 (SW-18010P) | `drv_vibration.c/h` |
| 4.4 | 전원 버튼/LED 드라이버 | `drv_button_led.c/h` |

### Phase 5: 시스템 서비스 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 5.1 | Config 관리자 (NVS 기반) | `config_manager.c/h` |
| 5.2 | 스케줄러 프레임워크 (Task/Cron) | `scheduler.c/h` |
| 5.3 | 전력 관리 (PM subsystem) | `power_manager.c/h` |
| 5.4 | 로그 시스템 (Flash 저장) | `log_manager.c/h` |
| 5.5 | Watchdog 관리 | `watchdog.c/h` |

### Phase 6: 애플리케이션 로직
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 6.1 | 센서 리딩 Task/Cron | `app_sensor.c/h` |
| 6.2 | 밸브 제어 Task | `app_valve.c/h` |
| 6.3 | 유량계 복합 제어 Task | `app_flowmeter.c/h` |
| 6.4 | 도난 방지 로직 | `app_security.c/h` |
| 6.5 | 상태 보고 Cron | `app_status.c/h` |

### Phase 7: OTA 및 안전 기능
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 7.1 | MCUboot 통합 (Dual-bank) | 부트로더 설정 |
| 7.2 | OTA 수신 (LoRa/BLE) | `ota_manager.c/h` |
| 7.3 | 공장 초기화 기능 | `factory_reset.c/h` |
| 7.4 | Fallback 로직 (밸브 안전) | `safety.c/h` |

### Phase 8: 통합 테스트 및 최적화
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 8.1 | 전체 기능 통합 테스트 | 테스트 리포트 |
| 8.2 | 전력 소모 측정 및 최적화 | 전력 프로파일 |
| 8.3 | 스트레스 테스트 | 안정성 리포트 |
| 8.4 | 문서화 | API 문서 |

---

## 4. 핀맵 요약 (RAK4630)

### 통신
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| UART1_RX (RS485) | P0.19 | uart1_rx |
| UART1_TX (RS485) | P0.20 | uart1_tx |
| RS485_DE | P1.04 | rs485_de |
| RS485_RE# | P1.03 | rs485_re |
| UART2_RX | P0.15 | uart2_rx |
| UART2_TX | P0.16 | uart2_tx |

### 모터 제어
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| X_EN_A | P0.14 | motor_x_en_a |
| X_EN_B | P0.13 | motor_x_en_b |
| X_EN_P2 | P0.04 | motor_x_en_p2 |
| Y_EN_A | P0.25 | motor_y_en_a |
| Y_EN_B | P1.01 | motor_y_en_b |
| Y_EN_P2 | P1.02 | motor_y_en_p2 |

### 센서/입력
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| DIO_X (유량계) | P0.10 | flowmeter_x |
| DIO_Y (유량계) | P0.09 | flowmeter_y |
| VIB_SENSE | P0.21 | vibration |
| BTN | P0.05 | button |
| BAT_AIN | P0.31 | battery_adc |

### 출력/제어
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| 12V_EN | P0.17 | pwr_12v_en |
| BUZZER_EN | P0.24 | buzzer |
| LED_EN | - | led (MCP23017) |

### Flash (QSPI)
| 기능 | 핀 |
|------|-----|
| QSPI_CS | P0.26 |
| QSPI_SCLK | P0.03 |
| QSPI_DIO0-3 | P0.30, P0.29, P0.28, P0.02 |

---

## 5. 디렉토리 구조

```
revita_link_fw/
├── CMakeLists.txt
├── prj.conf
├── Kconfig
├── boards/
│   └── rak4630_revita_link/
│       ├── rak4630_revita_link.dts
│       ├── rak4630_revita_link_defconfig
│       └── board.cmake
├── src/
│   ├── main.c
│   ├── hal/
│   │   ├── hal_gpio.c/h
│   │   ├── hal_uart.c/h
│   │   ├── hal_flash.c/h
│   │   ├── hal_adc.c/h
│   │   └── hal_buzzer.c/h
│   ├── drivers/
│   │   ├── drv_lora.c/h
│   │   ├── drv_ble.c/h
│   │   ├── drv_rs485.c/h
│   │   ├── drv_motor_valve.c/h
│   │   ├── drv_flowmeter.c/h
│   │   ├── drv_vibration.c/h
│   │   └── drv_button_led.c/h
│   ├── services/
│   │   ├── config_manager.c/h
│   │   ├── scheduler.c/h
│   │   ├── power_manager.c/h
│   │   ├── log_manager.c/h
│   │   └── watchdog.c/h
│   ├── app/
│   │   ├── app_sensor.c/h
│   │   ├── app_valve.c/h
│   │   ├── app_flowmeter.c/h
│   │   ├── app_security.c/h
│   │   └── app_status.c/h
│   ├── protocol/
│   │   ├── lora_mesh.c/h
│   │   └── modbus_rtu.c/h
│   └── ota/
│       └── ota_manager.c/h
├── include/
│   └── revita_link.h
├── dts/
│   └── bindings/
└── tests/
    └── ...
```

---

## 6. Zephyr 설정 (prj.conf 주요 항목)

```ini
# 기본 설정
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_HEAP_MEM_POOL_SIZE=16384

# GPIO
CONFIG_GPIO=y

# UART
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y

# SPI/QSPI Flash
CONFIG_SPI=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_NVS=y

# ADC
CONFIG_ADC=y

# LoRa (SX1262)
CONFIG_LORA=y
CONFIG_LORA_SX126X=y

# BLE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DIS=y

# 전력 관리
CONFIG_PM=y
CONFIG_PM_DEVICE=y

# Watchdog
CONFIG_WATCHDOG=y

# 로깅
CONFIG_LOG=y
CONFIG_LOG_BACKEND_UART=y

# MCUboot (OTA)
CONFIG_BOOTLOADER_MCUBOOT=y
CONFIG_IMG_MANAGER=y
```

---

## 7. GSD 워크플로우

### 7.1 진행 순서
```
1. /gsd:new-project        # 프로젝트 초기화
2. /gsd:map-codebase       # 코드베이스 분석 (초기)
3. /gsd:discuss-phase N    # 각 Phase 논의
4. /gsd:plan-phase N       # Phase 계획 수립
5. /gsd:execute-phase N    # Phase 실행
6. /gsd:verify-work N      # 검증
7. /gsd:next               # 다음 단계로
```

### 7.2 Phase 우선순위
```
[필수] Phase 1-3: 환경 + HAL + 통신 (기본 동작)
[필수] Phase 4-5: 주변장치 + 시스템 서비스
[필수] Phase 6: 애플리케이션 로직
[권장] Phase 7: OTA 및 안전 기능
[권장] Phase 8: 테스트 및 최적화
```

---

## 8. 위험 요소 및 대응

| 위험 | 영향 | 대응 |
|------|------|------|
| MeshCore 라이선스 | 상용 사용 제한 | 라이선스 확인, 대안 검토 |
| LoRa 통신 불안정 | 데이터 손실 | ACK/재전송 로직 강화 |
| 저전력 목표 미달 | 배터리 수명 감소 | PM 최적화, 측정 피드백 |
| Flash 용량 부족 | OTA 불가 | 코드 사이즈 최적화 |
| 밸브 오작동 | 현장 피해 | Fallback 로직 철저히 |

---

## 9. 참고 자료

- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
- [nRF52840 Product Specification](https://infocenter.nordicsemi.com/)
- [RAK4630 Datasheet](https://docs.rakwireless.com/)
- [SX1262 Datasheet](https://www.semtech.com/)
- [MeshCore GitHub](https://github.com/meshcore-dev/MeshCore)

---

*문서 버전: 1.0*
*작성일: 2026-03-24*
