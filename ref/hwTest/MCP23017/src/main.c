/*
 * MCP23017 GPA2/GPA3 + P0.04/P0.05 UART Control
 * I2C0: SCL=P0.14, SDA=P0.13
 * UART2: TX=P0.16, RX=P0.15
 *
 * '0' = STOP (all OFF)
 * '1' = P0.04 ON, P0.05 OFF, GPA3 ON, GPA2 OFF
 * '2' = P0.04 OFF, P0.05 ON, GPA3 OFF, GPA2 ON
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>

#define MCP23017_IODIRA   0x00
#define MCP23017_GPIOA    0x12
#define MCP23017_ADDR     0x20

#define GPA2_MASK         0x04  /* bit 2 */
#define GPA3_MASK         0x08  /* bit 3 */

#define PIN_04            4     /* P0.04 */
#define PIN_05            5     /* P0.05 */

static const struct device *i2c_dev;
static const struct device *uart2_dev;
static const struct device *gpio0_dev;

static void uart_send_string(const struct device *dev, const char *str)
{
    while (*str) {
        uart_poll_out(dev, *str++);
    }
}

static void print_msg(const char *msg)
{
    uart_send_string(uart2_dev, msg);
}

static int mcp23017_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev, buf, 2, MCP23017_ADDR);
}

int main(void)
{
    char buf[128];
    unsigned char c;
    int ret;
    bool p04_state = false;
    bool p05_state = false;
    uint8_t gpa_value = 0;
    uint32_t count = 0;
    int64_t last_tick = 0;
    int current_mode = 0;  /* 0=stop, 1=mode1, 2=mode2 */

    /* UART2 = uart0 in devicetree (TX=P0.16, RX=P0.15) */
    uart2_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    k_sleep(K_MSEC(100));

    print_msg("\r\n========================================\r\n");
    print_msg("MCP23017 GPA2/GPA3 + P0.04/P0.05 Control\r\n");
    print_msg("========================================\r\n");

    if (!device_is_ready(i2c_dev)) {
        print_msg("ERROR: I2C not ready\r\n");
        return -1;
    }
    print_msg("I2C: OK\r\n");

    if (!device_is_ready(gpio0_dev)) {
        print_msg("ERROR: GPIO0 not ready\r\n");
        return -1;
    }
    print_msg("GPIO0: OK\r\n");

    /* Configure P0.04 and P0.05 as output */
    gpio_pin_configure(gpio0_dev, PIN_04, GPIO_OUTPUT_LOW);
    gpio_pin_configure(gpio0_dev, PIN_05, GPIO_OUTPUT_LOW);

    /* Configure GPIOA: GPA0=input, GPA1-7=output */
    ret = mcp23017_write_reg(MCP23017_IODIRA, 0x01);
    if (ret < 0) {
        print_msg("ERROR: Cannot write IODIRA\r\n");
        return -1;
    }

    /* Initialize GPIOA to 0 */
    mcp23017_write_reg(MCP23017_GPIOA, 0x00);
    print_msg("MCP23017: OK\r\n\r\n");

    print_msg("Commands:\r\n");
    print_msg("  '0' = STOP (all OFF)\r\n");
    print_msg("  '1' = P0.04 ON, P0.05 OFF, GPA3 ON, GPA2 OFF\r\n");
    print_msg("  '2' = P0.04 OFF, P0.05 ON, GPA3 OFF, GPA2 ON\r\n\r\n");

    last_tick = k_uptime_get();

    while (1) {
        /* Poll for UART input */
        if (uart_poll_in(uart2_dev, &c) == 0) {
            if (c == '0') {
                p04_state = false;
                p05_state = false;
                gpa_value = 0x00;
                current_mode = 0;
                count = 0;
                gpio_pin_set(gpio0_dev, PIN_04, 0);
                gpio_pin_set(gpio0_dev, PIN_05, 0);
                mcp23017_write_reg(MCP23017_GPIOA, gpa_value);
                print_msg("\r\n>> STOP: All OFF\r\n");
            } else if (c == '1') {
                p04_state = true;
                p05_state = false;
                gpa_value = GPA3_MASK;  /* GPA3 ON, GPA2 OFF */
                current_mode = 1;
                count = 0;
                gpio_pin_set(gpio0_dev, PIN_04, 1);
                gpio_pin_set(gpio0_dev, PIN_05, 0);
                mcp23017_write_reg(MCP23017_GPIOA, gpa_value);
                print_msg("\r\n>> Mode 1: P0.04=ON, P0.05=OFF, GPA3=ON, GPA2=OFF\r\n");
            } else if (c == '2') {
                p04_state = false;
                p05_state = true;
                gpa_value = GPA2_MASK;  /* GPA3 OFF, GPA2 ON */
                current_mode = 2;
                count = 0;
                gpio_pin_set(gpio0_dev, PIN_04, 0);
                gpio_pin_set(gpio0_dev, PIN_05, 1);
                mcp23017_write_reg(MCP23017_GPIOA, gpa_value);
                print_msg("\r\n>> Mode 2: P0.04=OFF, P0.05=ON, GPA3=OFF, GPA2=ON\r\n");
            }
        }

        /* Count and display every 1 second */
        if (k_uptime_get() - last_tick >= 1000) {
            count++;
            snprintf(buf, sizeof(buf),
                "[%04u] Mode %d | P0.04=%d P0.05=%d | GPA2=%d GPA3=%d\r\n",
                count,
                current_mode,
                p04_state ? 1 : 0,
                p05_state ? 1 : 0,
                (gpa_value >> 2) & 1,
                (gpa_value >> 3) & 1);
            print_msg(buf);
            last_tick = k_uptime_get();
        }

        k_sleep(K_MSEC(10));
    }

    return 0;
}
