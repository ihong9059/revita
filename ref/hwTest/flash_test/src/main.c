/*
 * QSPI Flash Memory Test - MX25R1635F (nRF QSPI)
 * CS: P0.26, CLK: P0.03
 * DIO0-3: P0.30, P0.29, P0.28, P0.02
 * UART2: TX=P0.16
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>
#include <string.h>

#define FLASH_TEST_OFFSET  0x00000
#define FLASH_TEST_SIZE    256
#define FLASH_PAGE_SIZE    4096

static const struct device *uart_dev;
static const struct device *flash_dev;

static void uart_print(const char *str)
{
    while (*str) {
        uart_poll_out(uart_dev, *str++);
    }
}

static void print_hex(const uint8_t *data, size_t len)
{
    char buf[8];
    for (size_t i = 0; i < len; i++) {
        snprintf(buf, sizeof(buf), "%02X ", data[i]);
        uart_print(buf);
        if ((i + 1) % 16 == 0) {
            uart_print("\r\n");
        }
    }
    if (len % 16 != 0) {
        uart_print("\r\n");
    }
}

int main(void)
{
    int ret;
    char buf[128];
    uint8_t write_buf[FLASH_TEST_SIZE];
    uint8_t read_buf[FLASH_TEST_SIZE];

    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

    k_sleep(K_MSEC(100));

    uart_print("\r\n=== QSPI Flash Test (MX25R1635F) ===\r\n");

    /* Get flash device using mx25r64 label (overridden in overlay) */
    flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r64));
    if (!device_is_ready(flash_dev)) {
        uart_print("ERROR: Flash device not ready\r\n");
        return -1;
    }
    uart_print("Flash device ready (QSPI)\r\n");

    /* Read flash info */
    const struct flash_parameters *params = flash_get_parameters(flash_dev);
    if (params) {
        snprintf(buf, sizeof(buf), "Write block size: %u bytes\r\n",
                 (unsigned int)params->write_block_size);
        uart_print(buf);
        snprintf(buf, sizeof(buf), "Erase value: 0x%02X\r\n", params->erase_value);
        uart_print(buf);
    }

    /* Step 1: Read original data */
    uart_print("\r\n[1] Reading original data...\r\n");
    memset(read_buf, 0, sizeof(read_buf));
    ret = flash_read(flash_dev, FLASH_TEST_OFFSET, read_buf, 64);
    if (ret != 0) {
        snprintf(buf, sizeof(buf), "ERROR: Read failed (%d)\r\n", ret);
        uart_print(buf);
        return -1;
    }
    uart_print("Original data:\r\n");
    print_hex(read_buf, 64);

    /* Step 2: Erase sector */
    uart_print("\r\n[2] Erasing sector (4KB)...\r\n");
    ret = flash_erase(flash_dev, FLASH_TEST_OFFSET, FLASH_PAGE_SIZE);
    if (ret != 0) {
        snprintf(buf, sizeof(buf), "ERROR: Erase failed (%d)\r\n", ret);
        uart_print(buf);
        return -1;
    }
    uart_print("Erase OK\r\n");

    /* Step 3: Verify erase */
    uart_print("\r\n[3] Verifying erase...\r\n");
    memset(read_buf, 0, sizeof(read_buf));
    ret = flash_read(flash_dev, FLASH_TEST_OFFSET, read_buf, 64);
    if (ret != 0) {
        uart_print("ERROR: Read failed\r\n");
        return -1;
    }
    uart_print("After erase (should be 0xFF):\r\n");
    print_hex(read_buf, 64);

    /* Check if all 0xFF */
    bool erase_ok = true;
    for (int i = 0; i < 64; i++) {
        if (read_buf[i] != 0xFF) {
            erase_ok = false;
            break;
        }
    }
    uart_print(erase_ok ? "Erase verified OK\r\n" : "Erase verify FAILED\r\n");

    /* Step 4: Write test pattern */
    uart_print("\r\n[4] Writing test pattern...\r\n");
    for (int i = 0; i < FLASH_TEST_SIZE; i++) {
        write_buf[i] = (uint8_t)i;
    }

    ret = flash_write(flash_dev, FLASH_TEST_OFFSET, write_buf, FLASH_TEST_SIZE);
    if (ret != 0) {
        snprintf(buf, sizeof(buf), "ERROR: Write failed (%d)\r\n", ret);
        uart_print(buf);
        return -1;
    }
    uart_print("Write OK\r\n");

    /* Step 5: Read back and verify */
    uart_print("\r\n[5] Reading back data...\r\n");
    memset(read_buf, 0, sizeof(read_buf));
    ret = flash_read(flash_dev, FLASH_TEST_OFFSET, read_buf, FLASH_TEST_SIZE);
    if (ret != 0) {
        uart_print("ERROR: Read failed\r\n");
        return -1;
    }
    uart_print("Read data (first 64 bytes):\r\n");
    print_hex(read_buf, 64);

    /* Verify data */
    bool verify_ok = true;
    for (int i = 0; i < FLASH_TEST_SIZE; i++) {
        if (read_buf[i] != write_buf[i]) {
            verify_ok = false;
            snprintf(buf, sizeof(buf), "Mismatch at %d: wrote 0x%02X, read 0x%02X\r\n",
                     i, write_buf[i], read_buf[i]);
            uart_print(buf);
            break;
        }
    }

    uart_print("\r\n========================================\r\n");
    if (verify_ok) {
        uart_print("*** FLASH TEST PASSED ***\r\n");
    } else {
        uart_print("*** FLASH TEST FAILED ***\r\n");
    }
    uart_print("========================================\r\n");

    /* Keep running */
    uint32_t count = 0;
    while (1) {
        snprintf(buf, sizeof(buf), "[%u] Flash test complete\r\n", count++);
        uart_print(buf);
        k_sleep(K_SECONDS(5));
    }

    return 0;
}
