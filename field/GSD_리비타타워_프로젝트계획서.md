# GSD 프로젝트 계획서: 리비타 타워 펌웨어

> **프로젝트명**: REVITA_TOWER_FW
> **개발 도구**: Zephyr RTOS
> **타겟 MCU**: RAK4630 (nRF52840 + SX1262)
> **작성일**: 2026-03-24

---

## 1. 프로젝트 개요

### 1.1 제품 설명
리비타 타워는 노지 스마트팜용 관제 장치(중앙 허브)입니다.
- LTE로 서버와 MQTT 통신
- LoRa로 리비타 링크들과 통신 (마스터/게이트웨이)
- PTZ 카메라로 현장 촬영 (Core3506 연동)
- RS485 센서로 조도 데이터 수집
- GPS로 위치 정보 획득

### 1.2 핵심 목표
- 안정적인 LTE/MQTT 서버 통신
- LoRa Mesh 게이트웨이 역할 수행
- Core3506 프로세서와의 원활한 연동
- 동적 config 기반 운영
- Dual-bank OTA 지원

### 1.3 링크와의 주요 차이점
| 항목 | 리비타 링크 | 리비타 타워 |
|------|------------|------------|
| 역할 | 센서/제어 노드 | 중앙 허브/게이트웨이 |
| 서버 통신 | LoRa (타워 경유) | LTE/MQTT (직접) |
| LoRa 역할 | 슬레이브 | 마스터/게이트웨이 |
| 추가 모듈 | - | LTE, GPS, Core3506 |
| 카메라 | - | PTZ 카메라 (이더넷) |
| 전원 | 1S 인산철 | 12V 배터리 |
| 밸브/유량계 | 있음 | 없음 |

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
| 1.2 | RAK4630 커스텀 보드 DTS 작성 | `rak4630_revita_tower.dts` |
| 1.3 | 핀맵 정의 (devicetree overlay) | `revita_tower.overlay` |
| 1.4 | 기본 빌드 및 플래싱 테스트 | Hello World 동작 확인 |

### Phase 2: HAL 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 2.1 | GPIO 드라이버 (LED, 버튼, 전원 제어) | `hal_gpio.c/h` |
| 2.2 | UART 드라이버 (UART1, UART2) | `hal_uart.c/h` |
| 2.3 | QSPI Flash 드라이버 (MX25R1635F) | `hal_flash.c/h` |
| 2.4 | ADC 드라이버 (배터리 전압) | `hal_adc.c/h` |
| 2.5 | PWM/GPIO 드라이버 (부저) | `hal_buzzer.c/h` |
| 2.6 | I2C 드라이버 (MCP23017) | `hal_i2c.c/h` |

### Phase 3: 통신 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 3.1 | LoRa 드라이버 (SX1262) | `drv_lora.c/h` |
| 3.2 | LoRa Mesh 통합 (MeshCore 게이트웨이) | `lora_mesh.c/h` |
| 3.3 | BLE 드라이버 (Nordic SoftDevice) | `drv_ble.c/h` |
| 3.4 | RS485 드라이버 (UART + MUX) | `drv_rs485.c/h` |
| 3.5 | MODBUS RTU 프로토콜 | `modbus_rtu.c/h` |

### Phase 4: LTE/GPS 드라이버 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 4.1 | LTE 모뎀 드라이버 (RM76-4) | `drv_lte.c/h` |
| 4.2 | AT 명령어 파서 | `at_parser.c/h` |
| 4.3 | MQTT 클라이언트 | `mqtt_client.c/h` |
| 4.4 | GPS 드라이버 | `drv_gps.c/h` |
| 4.5 | 시간 동기화 (NTP/GPS) | `time_sync.c/h` |

### Phase 5: 프로세서 연동 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 5.1 | Core3506 UART 통신 프로토콜 | `core3506_comm.c/h` |
| 5.2 | 프로세서 전원 제어 | `drv_processor.c/h` |
| 5.3 | 카메라 촬영 명령 중계 | `camera_control.c/h` |
| 5.4 | UART MUX 제어 (RS485 공유) | `uart_mux.c/h` |

### Phase 6: 시스템 서비스 개발
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 6.1 | Config 관리자 (NVS 기반) | `config_manager.c/h` |
| 6.2 | 스케줄러 프레임워크 (Task/Cron) | `scheduler.c/h` |
| 6.3 | 전력 관리 (PM subsystem) | `power_manager.c/h` |
| 6.4 | 로그 시스템 (Flash 저장 + 원격 전송) | `log_manager.c/h` |
| 6.5 | Watchdog 관리 | `watchdog.c/h` |

### Phase 7: 애플리케이션 로직
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 7.1 | 센서 리딩 Cron (조도 센서) | `app_sensor.c/h` |
| 7.2 | LoRa 메시지 라우팅 (게이트웨이) | `app_gateway.c/h` |
| 7.3 | 서버 통신 Task (MQTT 업링크/다운링크) | `app_server.c/h` |
| 7.4 | 카메라 촬영 Task/Cron | `app_camera.c/h` |
| 7.5 | 도난 방지 로직 | `app_security.c/h` |
| 7.6 | 상태 보고 Cron | `app_status.c/h` |

### Phase 8: OTA 및 안전 기능
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 8.1 | MCUboot 통합 (Dual-bank) | 부트로더 설정 |
| 8.2 | OTA 수신 (LTE/BLE) | `ota_manager.c/h` |
| 8.3 | 링크 OTA 중계 (LoRa) | `ota_relay.c/h` |
| 8.4 | 공장 초기화 기능 | `factory_reset.c/h` |

### Phase 9: 통합 테스트 및 최적화
| 순서 | 작업 | 산출물 |
|------|------|--------|
| 9.1 | 전체 기능 통합 테스트 | 테스트 리포트 |
| 9.2 | LTE/LoRa 동시 운영 테스트 | 통신 안정성 리포트 |
| 9.3 | 장시간 운영 테스트 | 안정성 리포트 |
| 9.4 | 문서화 | API 문서 |

---

## 4. 핀맵 요약 (리비타 타워)

### 통신 (UART)
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| UART1_RX (RS485/LTE) | P0.19 | uart1_rx |
| UART1_TX (RS485/LTE) | P0.20 | uart1_tx |
| UART2_RX (Core3506) | P0.15 | uart2_rx |
| UART2_TX (Core3506) | P0.16 | uart2_tx |
| RS485_DE | P1.04 | rs485_de |
| RS485_RE# | P1.03 | rs485_re |

### UART MUX 제어
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| MUX_SEL | TBD | uart_mux_sel |

### 전원 제어
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| LTE_PWR_EN | TBD | lte_pwr_en |
| PROC_PWR_EN | TBD | proc_pwr_en |
| 12V_EN | P0.17 | pwr_12v_en |

### 센서/입력
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
| VIB_SENSE | P0.21 | vibration |
| BTN | P0.05 | button |
| BAT_AIN | P0.31 | battery_adc |

### 출력/제어
| 기능 | 핀 | Zephyr alias |
|------|-----|--------------|
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
revita_tower_fw/
├── CMakeLists.txt
├── prj.conf
├── Kconfig
├── boards/
│   └── rak4630_revita_tower/
│       ├── rak4630_revita_tower.dts
│       ├── rak4630_revita_tower_defconfig
│       └── board.cmake
├── src/
│   ├── main.c
│   ├── hal/
│   │   ├── hal_gpio.c/h
│   │   ├── hal_uart.c/h
│   │   ├── hal_flash.c/h
│   │   ├── hal_adc.c/h
│   │   ├── hal_buzzer.c/h
│   │   └── hal_i2c.c/h
│   ├── drivers/
│   │   ├── drv_lora.c/h
│   │   ├── drv_ble.c/h
│   │   ├── drv_rs485.c/h
│   │   ├── drv_lte.c/h
│   │   ├── drv_gps.c/h
│   │   ├── drv_processor.c/h
│   │   ├── drv_vibration.c/h
│   │   └── drv_button_led.c/h
│   ├── services/
│   │   ├── config_manager.c/h
│   │   ├── scheduler.c/h
│   │   ├── power_manager.c/h
│   │   ├── log_manager.c/h
│   │   ├── time_sync.c/h
│   │   └── watchdog.c/h
│   ├── app/
│   │   ├── app_sensor.c/h
│   │   ├── app_gateway.c/h
│   │   ├── app_server.c/h
│   │   ├── app_camera.c/h
│   │   ├── app_security.c/h
│   │   └── app_status.c/h
│   ├── protocol/
│   │   ├── lora_mesh.c/h
│   │   ├── modbus_rtu.c/h
│   │   ├── mqtt_client.c/h
│   │   ├── at_parser.c/h
│   │   └── core3506_comm.c/h
│   └── ota/
│       ├── ota_manager.c/h
│       └── ota_relay.c/h
├── include/
│   └── revita_tower.h
├── dts/
│   └── bindings/
└── tests/
    └── ...
```

---

## 6. Zephyr 설정 (prj.conf 주요 항목)

```ini
# 기본 설정
CONFIG_MAIN_STACK_SIZE=8192
CONFIG_HEAP_MEM_POOL_SIZE=32768

# GPIO
CONFIG_GPIO=y

# UART
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_UART_ASYNC_API=y

# SPI/QSPI Flash
CONFIG_SPI=y
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_NVS=y

# I2C (MCP23017)
CONFIG_I2C=y

# ADC
CONFIG_ADC=y

# LoRa (SX1262)
CONFIG_LORA=y
CONFIG_LORA_SX126X=y

# BLE
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DIS=y

# 네트워크 (LTE/MQTT)
CONFIG_NETWORKING=y
CONFIG_NET_SOCKETS=y
CONFIG_MQTT_LIB=y

# Modem (LTE)
CONFIG_MODEM=y
CONFIG_MODEM_CELLULAR=y

# 전력 관리
CONFIG_PM=y
CONFIG_PM_DEVICE=y

# Watchdog
CONFIG_WATCHDOG=y

# 로깅
CONFIG_LOG=y
CONFIG_LOG_BACKEND_UART=y
CONFIG_LOG_BACKEND_NET=y

# MCUboot (OTA)
CONFIG_BOOTLOADER_MCUBOOT=y
CONFIG_IMG_MANAGER=y

# 시간/RTC
CONFIG_RTC=y
```

---

## 7. 통신 아키텍처

### 7.1 전체 구조
```
┌─────────────────────────────────────────────────────────────┐
│                        서버 (클라우드)                        │
└─────────────────────────┬───────────────────────────────────┘
                          │ LTE/MQTT
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                      리비타 타워                              │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  RAK4630    │  │   RM76-4    │  │     Core3506        │  │
│  │ (nRF52840)  │──│   (LTE)     │  │   (프로세서)        │  │
│  │             │  │             │  │                     │  │
│  │ - LoRa GW   │  │ - MQTT      │  │ - PTZ 카메라 제어   │  │
│  │ - BLE       │  │ - GPS       │  │ - 영상 스트리밍     │  │
│  │ - 스케줄러  │  └─────────────┘  └─────────────────────┘  │
│  └──────┬──────┘                                            │
│         │ LoRa Mesh                                         │
└─────────┼───────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────┐     ┌─────────────────────┐
│   리비타 링크 #1    │     │   리비타 링크 #2    │
│   - 센서            │     │   - 밸브 제어       │
│   - 유량계          │     │   - 센서            │
└─────────────────────┘     └─────────────────────┘
```

### 7.2 MQTT 토픽 구조 (예시)
```
revita/tower/{device_id}/status    # 타워 상태 업링크
revita/tower/{device_id}/command   # 서버 → 타워 명령
revita/link/{device_id}/data       # 링크 센서 데이터 (타워 경유)
revita/link/{device_id}/control    # 링크 제어 명령 (타워 경유)
```

---

## 8. GSD 워크플로우

### 8.1 진행 순서
```
1. /gsd:new-project        # 프로젝트 초기화
2. /gsd:map-codebase       # 코드베이스 분석 (초기)
3. /gsd:discuss-phase N    # 각 Phase 논의
4. /gsd:plan-phase N       # Phase 계획 수립
5. /gsd:execute-phase N    # Phase 실행
6. /gsd:verify-work N      # 검증
7. /gsd:next               # 다음 단계로
```

### 8.2 Phase 우선순위
```
[필수] Phase 1-3: 환경 + HAL + 통신 (기본 동작)
[필수] Phase 4: LTE/GPS (서버 연결)
[필수] Phase 5: 프로세서 연동 (카메라)
[필수] Phase 6-7: 시스템 서비스 + 애플리케이션
[권장] Phase 8: OTA 및 안전 기능
[권장] Phase 9: 테스트 및 최적화
```

---

## 9. 위험 요소 및 대응

| 위험 | 영향 | 대응 |
|------|------|------|
| LTE 연결 불안정 | 서버 통신 두절 | 재연결 로직, 로컬 버퍼링 |
| LoRa/LTE 동시 운영 간섭 | 통신 충돌 | 시분할 스케줄링 |
| Core3506 응답 지연 | 카메라 동작 실패 | 타임아웃 처리, 재시도 |
| MeshCore 라이선스 | 상용 사용 제한 | 라이선스 확인, 대안 검토 |
| Flash 용량 부족 | OTA 불가 | 코드 사이즈 최적화 |
| 전력 관리 복잡성 | 배터리 수명 감소 | 각 모듈별 전원 제어 |

---

## 10. 리비타 링크와의 코드 공유

### 10.1 공유 가능 모듈
```
- hal/: hal_gpio, hal_uart, hal_flash, hal_adc, hal_buzzer
- drivers/: drv_lora, drv_ble, drv_rs485, drv_vibration, drv_button_led
- services/: config_manager, scheduler, watchdog
- protocol/: lora_mesh (기본), modbus_rtu
- ota/: ota_manager (수신부)
```

### 10.2 타워 전용 모듈
```
- drivers/: drv_lte, drv_gps, drv_processor
- services/: time_sync (NTP/GPS)
- app/: app_gateway, app_server, app_camera
- protocol/: mqtt_client, at_parser, core3506_comm
- ota/: ota_relay (링크 중계)
```

---

## 11. 참고 자료

- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
- [nRF52840 Product Specification](https://infocenter.nordicsemi.com/)
- [RAK4630 Datasheet](https://docs.rakwireless.com/)
- [SX1262 Datasheet](https://www.semtech.com/)
- [MeshCore GitHub](https://github.com/meshcore-dev/MeshCore)
- [RM76-4 AT Command Manual](https://www.quectel.com/)

---

*문서 버전: 1.0*
*작성일: 2026-03-24*
