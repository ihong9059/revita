cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kc_firmware)

target_sources(app PRIVATE
    src/main.c
    src/modbus.c
    src/rs485.c
    src/registers.c
    src/drivers.c
)
