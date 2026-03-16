# bleTest - BLE Communication Test

## Overview

| Role | Project | Target | J-Link SN |
|------|---------|--------|-----------|
| RX (Peripheral) | peripheral/ | Target1 | 001050234191 |
| TX (Central) | central/ | Target2 | 1050295470 |

## Features

- Counter: 1초마다 1씩 증가
- RSSI 표시
- TX Power 표시 (+8 dBm)

## BLE Configuration

| Parameter | Value |
|-----------|-------|
| Service UUID | 12345678-1234-5678-1234-56789abcdef0 |
| RX Char UUID | 12345678-1234-5678-1234-56789abcdef1 |
| Device Name (Peripheral) | REVITA_RX |
| Device Name (Central) | REVITA_TX |
| TX Power | +8 dBm |

## Build

```bash
cd /home/uttec/ncs
source .venv/bin/activate
export ZEPHYR_BASE=/home/uttec/ncs/zephyr
export GNUARMEMB_TOOLCHAIN_PATH=/home/uttec/gnuarmemb
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb

# Peripheral (Target1 - RX)
west build -b nrf52840dk/nrf52840 /home/uttec/revita/zephyr_workspace/bleTest/peripheral \
    --build-dir /home/uttec/revita/zephyr_workspace/bleTest/peripheral/build

# Central (Target2 - TX)
west build -b nrf52840dk/nrf52840 /home/uttec/revita/zephyr_workspace/bleTest/central \
    --build-dir /home/uttec/revita/zephyr_workspace/bleTest/central/build
```

## Flash

```bash
# Peripheral (Target1)
west flash -d /home/uttec/revita/zephyr_workspace/bleTest/peripheral/build --snr 001050234191

# Central (Target2)
west flash -d /home/uttec/revita/zephyr_workspace/bleTest/central/build --snr 1050295470
```

## Output Example

### Peripheral (RX)
```
=================================
BLE Peripheral (RX) - Target1
TX Power: +8 dBm
=================================

Bluetooth initialized
Advertising started as "REVITA_RX"
Waiting for connection...
Connected
RX: 1 | RSSI: -45 dBm | TX Power: +8 dBm
RX: 2 | RSSI: -44 dBm | TX Power: +8 dBm
...
```

### Central (TX)
```
=================================
BLE Central (TX) - Target2
TX Power: +8 dBm
=================================

Bluetooth initialized
Scanning for REVITA_RX...
Found REVITA_RX at XX:XX:XX:XX:XX:XX (RSSI -45)
Connected
Discovery started
RX Characteristic found (handle: 17)
Discovery complete
TX: 1 | RSSI: -45 dBm | TX Power: +8 dBm
TX: 2 | RSSI: -44 dBm | TX Power: +8 dBm
...
```

## Memory Usage

| Project | FLASH | RAM |
|---------|-------|-----|
| Peripheral | 144 KB (13.78%) | 28 KB (10.89%) |
| Central | 152 KB (14.50%) | 29 KB (11.06%) |
