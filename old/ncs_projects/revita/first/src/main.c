#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* UART2: TX P0.16, RX P0.15 -> mapped to uart0 */
static const struct device *uart2_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* UART1: TX P0.20, RX P0.19 -> mapped to uart1 */
static const struct device *uart1_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

static void uart_send_string(const struct device *dev, const char *str)
{
    while (*str) {
        uart_poll_out(dev, *str++);
    }
}

int main(void)
{
    uint32_t counter = 0;
    char buf[64];

    if (!device_is_ready(uart2_dev)) {
        return -1;
    }

    if (!device_is_ready(uart1_dev)) {
        return -1;
    }

    while (1) {
        snprintf(buf, sizeof(buf), "UART2 count: %u\r\n", counter);
        uart_send_string(uart2_dev, buf);

        snprintf(buf, sizeof(buf), "UART1 count: %u\r\n", counter);
        uart_send_string(uart1_dev, buf);

        counter++;
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
