# PCA10059 (nRF52840 Dongle) 외부 디버거 프로그래밍 가이드

## 개요

PCA10056 (nRF52840 DK)를 프로그래머로 사용하여 PCA10059 (nRF52840 Dongle)를 SWD로 프로그래밍할 때 발생하는 문제들과 해결 방법을 정리합니다.

nRF Connect SDK (Zephyr 기반) 환경에서 작업합니다.

---

## 하드웨어 연결

- **PCA10056** (nRF52840 DK): USB로 PC에 연결, J-Link OB 프로그래머 역할
- **PCA10059** (nRF52840 Dongle): PCA10056의 **Debug Out** 커넥터에 SWD 케이블로 연결
- PCA10056의 디버그 타겟 스위치가 **EXT** (외부)로 설정되어야 함
- PCA10059는 자체 USB 전원 또는 Debug Out 커넥터를 통해 전원 공급 (VTref=3.3V 확인)

---

## 문제 1: Zephyr SDK 미설치 시 빌드 실패

### 증상
```
Could not find a package configuration file provided by "Zephyr-sdk"
```

### 원인
`ZEPHYR_SDK_INSTALL_DIR` 환경변수가 존재하지 않는 경로를 가리키거나, Zephyr SDK가 설치되지 않은 경우.

### 해결
GNU Arm Embedded Toolchain을 사용하도록 환경변수 설정:
```bash
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/path/to/arm-gnu-toolchain
unset ZEPHYR_SDK_INSTALL_DIR
```

Raspberry Pi 4에서는:
```bash
# ARM GNU Toolchain 설치
sudo apt install gcc-arm-none-eabi

# 환경변수 설정 (~/.bashrc에 추가)
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/usr
```

---

## 문제 2: Flash Load Offset 불일치 (가장 핵심 문제)

### 증상
- `nrfjprog --program --verify` 성공
- 하지만 LED 등 어떤 동작도 없음
- J-Link으로 확인 시 PC=0xFFFFFFFE, IPSR=HardFault

### 원인
PCA10059 보드 정의에 `CONFIG_BOARD_HAS_NRF5_BOOTLOADER=y`가 설정되어 있어서, 기본적으로 **FLASH_LOAD_OFFSET=0x1000** 으로 빌드됩니다.

이는 Nordic MBR (Master Boot Record)이 0x0~0x1000에 있고, 앱이 0x1000부터 시작하는 구조입니다. 하지만 `nrfjprog --recover`로 칩 전체를 지우면 **MBR도 삭제**되어 0x0에 유효한 벡터 테이블이 없게 됩니다.

CPU는 부팅 시:
1. 0x0에서 Initial SP 로드 → 0xFFFFFFFF (빈 플래시)
2. 0x4에서 Reset Vector 로드 → 0xFFFFFFFF
3. 0xFFFFFFFE로 점프 → **HardFault**

### 해결
**방법 A: DT 코드 파티션 사용 (권장)**

`prj.conf`에 추가:
```
CONFIG_USE_DT_CODE_PARTITION=y
```

보드 오버레이 파일 (`boards/nrf52840dongle_nrf52840.overlay`)에서 파티션을 0x0부터 시작하도록 변경:
```dts
/ {
	chosen {
		zephyr,code-partition = &slot0_partition;
	};
};

&flash0 {
	/delete-node/ partitions;

	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;

		slot0_partition: partition@0 {
			label = "image-0";
			reg = <0x00000000 0x000f8000>;
		};
		storage_partition: partition@f8000 {
			label = "storage";
			reg = <0x000f8000 0x00008000>;
		};
	};
};
```

**방법 B: FLASH_LOAD_OFFSET 직접 설정**

`prj.conf`에 추가:
```
CONFIG_FLASH_LOAD_OFFSET=0x0
```

### 확인 방법
빌드 후 `.config` 파일에서 확인:
```bash
grep FLASH_LOAD_OFFSET build/zephyr/.config
# CONFIG_FLASH_LOAD_OFFSET=0x0 이어야 함
```

hex 파일 첫 줄에서 주소 확인:
```
:10000000C018002095080000...   ← 0x0000 (정상)
:10100000C018002095080000...   ← 0x1000 (문제! MBR 없으면 부팅 불가)
```

---

## 문제 3: nRF52840 Rev2 APPROTECT

### 증상
- `nrfjprog --program --verify` 성공
- `nrfjprog --memrd 0x00000000` → 전부 0xFFFFFFFF
- `nrfjprog --reset` → "Access protection is enabled, can't start device"
- J-Link 연결 시 FICR.INFO.PART도 0xFFFFFFFF로 읽힘

### 원인
nRF52840 Rev2 (xxAA_REV2)에서 UICR.APPROTECT (0x10001208)가 erased 상태(0xFFFFFFFF)이면 **APPROTECT가 활성화**됩니다. (구형 리비전에서는 0xFF=비활성화였음)

`nrfjprog --recover` 후 UICR이 전부 지워져서 APPROTECT=0xFFFFFFFF가 되고, 리셋 후 디버그 접근이 차단됩니다.

### 해결
`nrfjprog --recover` 후 즉시 APPROTECT를 비활성화:
```bash
nrfjprog --recover
nrfjprog --memwr 0x10001208 --val 0x0000005A
nrfjprog --program firmware.hex --verify --reset
```

> **참고:** 리셋 후 "Access protection is enabled" 에러가 나더라도, 이는 **디버그 접근만** 차단되는 것입니다. CPU는 자체적으로 플래시에서 코드를 읽고 실행합니다. 코드가 올바르게 주소 0x0에 배치되어 있으면 정상 동작합니다.

> **참고:** Zephyr의 `CONFIG_NRF_APPROTECT_USE_UICR=y` 설정이 활성화되어 있으면, 펌웨어가 부팅 시 UICR.APPROTECT 값을 읽어 런타임에 APPROTECT.DISABLE 레지스터에 반영합니다.

---

## 문제 4: USB CDC ACM 콘솔 의존성

### 증상
`CONFIG_USB_DEVICE_STACK=n`으로 USB를 비활성화하면 링커 에러:
```
undefined reference to `__device_dts_ord_112'
```

### 원인
PCA10059 보드 정의에서 기본 콘솔이 USB CDC ACM UART로 설정되어 있음:
```dts
chosen {
    zephyr,console = &cdc_acm_uart;
};
```
USB를 비활성화해도 콘솔 드라이버가 USB UART 디바이스를 참조합니다.

### 해결
USB와 함께 콘솔/시리얼도 비활성화:
```
CONFIG_USB_DEVICE_STACK=n
CONFIG_SERIAL=n
CONFIG_CONSOLE=n
CONFIG_UART_CONSOLE=n
```

---

## 최종 프로젝트 구성

### 디렉토리 구조
```
pca10059Test/
├── CMakeLists.txt
├── prj.conf
├── boards/
│   └── nrf52840dongle_nrf52840.overlay
└── src/
    └── main.c
```

### CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pca10059Test)

target_sources(app PRIVATE src/main.c)
```

### prj.conf
```
CONFIG_GPIO=y
CONFIG_USE_DT_CODE_PARTITION=y
CONFIG_USB_DEVICE_STACK=n
CONFIG_SERIAL=n
CONFIG_CONSOLE=n
CONFIG_UART_CONSOLE=n
```

### boards/nrf52840dongle_nrf52840.overlay
```dts
/ {
	chosen {
		zephyr,code-partition = &slot0_partition;
	};
};

&flash0 {
	/delete-node/ partitions;

	partitions {
		compatible = "fixed-partitions";
		#address-cells = <1>;
		#size-cells = <1>;

		slot0_partition: partition@0 {
			label = "image-0";
			reg = <0x00000000 0x000f8000>;
		};
		storage_partition: partition@f8000 {
			label = "storage";
			reg = <0x000f8000 0x00008000>;
		};
	};
};
```

### PCA10059 LED 핀 배치
| Alias | 색상 | GPIO | 비고 |
|-------|------|------|------|
| led0 | Green | P0.06 | Active Low |
| led1 | Red | P0.08 | Active Low |
| led2 | Green | P1.09 | Active Low |
| led3 | Blue | P0.12 | Active Low |

---

## 빌드 & 플래시 명령어 (공통)

### 빌드
```bash
west build -b nrf52840dongle/nrf52840 . --no-sysbuild
```

> `--no-sysbuild` 필수: sysbuild 사용 시 별도의 sysbuild 설정이 필요함

### 플래시 (외부 디버거 사용 시)
```bash
# 1. 칩 복구 (APPROTECT 해제 + 전체 삭제)
nrfjprog --recover

# 2. APPROTECT 비활성화 (nRF52840 Rev2 필수)
nrfjprog --memwr 0x10001208 --val 0x0000005A

# 3. 프로그래밍 + 검증 + 리셋
nrfjprog --program build/zephyr/zephyr.hex --verify --reset
```

> 리셋 후 "Access protection is enabled" 에러는 무시해도 됩니다. 코드는 정상 실행됩니다.

---

## Raspberry Pi 4 환경 설정 참고

### 필요 패키지
```bash
sudo apt update
sudo apt install -y python3-pip python3-venv git cmake ninja-build \
    gcc-arm-none-eabi gperf dfu-util device-tree-compiler wget

pip3 install west
```

### nRF Connect SDK 설치
```bash
mkdir ~/ncs && cd ~/ncs
west init -m https://github.com/nrfconnect/sdk-nrf
west update
```

### nRF Command Line Tools (ARM64)
Nordic 공식 사이트에서 ARM64용 nrf-command-line-tools 다운로드 설치:
```bash
# 설치 후 확인
nrfjprog --version
```

### J-Link 설치
SEGGER 공식 사이트에서 ARM64 Linux용 J-Link 다운로드 설치:
```bash
# 설치 후 확인
JLinkExe --version
```

### USB 권한 설정
```bash
# J-Link USB 권한 (보통 J-Link 설치 시 자동 설정)
sudo cp 99-jlink.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
```
