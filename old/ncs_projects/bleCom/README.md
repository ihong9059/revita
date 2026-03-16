# bleCom - BLE UART Loopback

PC와 PCA10040(nRF52832 DK) 간 BLE 통신 프로젝트.
HyperTerminal에서 입력한 문장이 BLE를 통해 PC로 전송되고, PC에서 다시 BLE로 echo back되어 HyperTerminal에 출력된다.

## 구조

```
HyperTerminal (COM3, 115200)
    ↕ UART
PCA10040 (BLE Peripheral)
    ↕ BLE
PC Python (BLE Central - loopback)
```

## 디렉토리

```
bleCom/
├── peripheral/   # Zephyr 펌웨어 (PCA10040)
│   ├── CMakeLists.txt
│   ├── prj.conf
│   └── src/main.c
├── central/      # Python 스크립트 (PC)
│   └── central.py
└── README.md
```

## BLE 서비스

Nordic UART Service (NUS) 호환 UUID 사용:

| 항목 | UUID |
|------|------|
| Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` |
| TX (notify, peripheral→central) | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` |
| RX (write, central→peripheral) | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` |

## LED 상태 (PCA10040)

| LED | 의미 |
|-----|------|
| LED0 | BLE Advertising 중 |
| LED1 | BLE 연결됨 |
| LED2 | UART 데이터 수신 시 토글 |
| LED3 | BLE 데이터 수신 시 토글 |

## Peripheral 빌드 & 플래시

```bash
# 환경 설정
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/path/to/arm-gnu-toolchain
export ZEPHYR_BASE=/path/to/ncs/zephyr
unset ZEPHYR_SDK_INSTALL_DIR

# 빌드
cd bleCom/peripheral
west build -b nrf52dk/nrf52832 . --no-sysbuild

# 플래시
west flash
```

## Central 실행 (PC)

```bash
pip install bleak
python central.py
```

## 사용법

1. Peripheral 펌웨어를 PCA10040에 플래시
2. HyperTerminal에서 J-Link CDC UART 포트 (COM3) 연결, 115200 baud
3. `python central.py` 실행 → BLE 연결 자동
4. HyperTerminal에서 문장 입력 + Enter → echo back 확인

## 해결한 문제들

### 1. UART console과 데이터 UART 충돌
- PCA10040의 uart0은 J-Link CDC UART를 통해 PC로 연결됨
- Zephyr console이 uart0을 점유하면 데이터 송수신 불가
- **해결**: `CONFIG_CONSOLE=n`, `CONFIG_UART_CONSOLE=n` 설정, LED로 상태 표시

### 2. Windows BLE GATT 캐시
- 서비스 구조 변경 후 Windows가 이전 GATT DB를 캐시하여 연결 실패
- **해결**: NUS 호환 UUID로 변경하여 캐시 우회

### 3. ISR에서 bt_gatt_notify 호출 실패
- UART ISR 안에서 BLE 함수를 직접 호출하면 동작하지 않음
- **해결**: ISR에서는 세마포어로 신호만 주고, main thread에서 bt_gatt_notify 호출

### 4. 20바이트 이상 전송 실패
- BLE 기본 MTU 23 (payload 20바이트) 초과 시 데이터 유실
- bt_gatt_notify가 TX 버퍼 부족으로 두 번째 chunk 전송 실패
- **해결**:
  - `CONFIG_BT_L2CAP_TX_MTU=247` (MTU 확장)
  - `CONFIG_BT_L2CAP_TX_BUF_COUNT=10` (TX 버퍼 증가)
  - 연결 시 `bt_gatt_exchange_mtu()` 호출
  - notify 실패 시 재시도 로직 (최대 10회, 10ms 간격)

## prj.conf 주요 설정

```
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_DEVICE_NAME="bleCom"
CONFIG_BT_L2CAP_TX_MTU=247
CONFIG_BT_BUF_ACL_TX_SIZE=251
CONFIG_BT_BUF_ACL_RX_SIZE=251
CONFIG_BT_CTLR_DATA_LENGTH_MAX=251
CONFIG_BT_L2CAP_TX_BUF_COUNT=10
CONFIG_BT_ATT_TX_COUNT=10
CONFIG_BT_GATT_CLIENT=y
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_CONSOLE=n
CONFIG_UART_CONSOLE=n
```
