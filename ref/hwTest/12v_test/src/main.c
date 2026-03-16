/*
 * 12V_EN Test
 * MCP23017 GPA1 = 12V_EN
 * P0.10 = DIO_X (should go LOW when 12V_EN is HIGH)
 * UART2: TX=P0.16
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>

/* MCP23017 Registers */
#define MCP23017_IODIRA   0x00
#define MCP23017_IODIRB   0x01
#define MCP23017_GPIOA    0x12
#define MCP23017_GPIOB    0x13
#define MCP23017_ADDR     0x20

/* MCP23017 GPA bits */
#define GPA_BUZZER_EN  (1 << 0)
#define GPA_12V_EN     (1 << 1)
#define GPA_X_EN_A     (1 << 2)
#define GPA_X_EN_B     (1 << 3)
#define GPA_X_EN_P2    (1 << 4)
#define GPA_Y_EN_A     (1 << 5)
#define GPA_Y_EN_B     (1 << 6)
#define GPA_Y_EN_P2    (1 << 7)

/* DIO_X pin */
#define DIO_X_PIN      10

static const struct device *i2c_dev;
static const struct device *uart_dev;
static const struct device *gpio0_dev;

static void uart_print(const char *str)
{
    while (*str) {
        uart_poll_out(uart_dev, *str++);
    }
}

static int mcp23017_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write(i2c_dev, buf, 2, MCP23017_ADDR);
}

static int mcp23017_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_write_read(i2c_dev, MCP23017_ADDR, &reg, 1, value, 1);
}

int main(void)
{
    int ret;
    char buf[128];
    uint8_t gpa_val;
    int dio_x_val;

    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    k_sleep(K_MSEC(100));

    uart_print("\r\n=== 12V_EN Test ===\r\n");
    uart_print("GPA1 = 12V_EN, P0.10 = DIO_X\r\n\r\n");

    if (!device_is_ready(i2c_dev)) {
        uart_print("ERROR: I2C not ready\r\n");
        return -1;
    }

    if (!device_is_ready(gpio0_dev)) {
        uart_print("ERROR: GPIO0 not ready\r\n");
        return -1;
    }

    /* Configure P0.10 as input */
    ret = gpio_pin_configure(gpio0_dev, DIO_X_PIN, GPIO_INPUT);
    if (ret != 0) {
        uart_print("ERROR: Failed to configure P0.10\r\n");
        return -1;
    }
    uart_print("P0.10 configured as input\r\n");

    /* Configure MCP23017 Port A as output */
    ret = mcp23017_write_reg(MCP23017_IODIRA, 0x00);
    if (ret != 0) {
        uart_print("ERROR: Failed to set IODIRA\r\n");
        return -1;
    }
    uart_print("MCP23017 Port A = output\r\n");

    /* Initial state: 12V_EN = LOW */
    ret = mcp23017_write_reg(MCP23017_GPIOA, 0x00);
    if (ret != 0) {
        uart_print("ERROR: Failed to write GPIOA\r\n");
        return -1;
    }

    uart_print("\r\n--- Test Start ---\r\n\r\n");

    /* Step 1: 12V_EN = LOW, read DIO_X */
    uart_print("[1] 12V_EN = LOW\r\n");
    mcp23017_write_reg(MCP23017_GPIOA, 0x00);
    k_sleep(K_MSEC(100));

    dio_x_val = gpio_pin_get(gpio0_dev, DIO_X_PIN);
    snprintf(buf, sizeof(buf), "    DIO_X (P0.10) = %s\r\n",
             dio_x_val ? "HIGH" : "LOW");
    uart_print(buf);

    k_sleep(K_SECONDS(2));

    /* Step 2: 12V_EN = HIGH, read DIO_X */
    uart_print("\r\n[2] 12V_EN = HIGH\r\n");
    mcp23017_write_reg(MCP23017_GPIOA, GPA_12V_EN);
    k_sleep(K_MSEC(500));  /* Wait for 12V to stabilize */

    dio_x_val = gpio_pin_get(gpio0_dev, DIO_X_PIN);
    snprintf(buf, sizeof(buf), "    DIO_X (P0.10) = %s\r\n",
             dio_x_val ? "HIGH" : "LOW");
    uart_print(buf);

    /* Check expected result */
    if (dio_x_val == 0) {
        uart_print("\r\n*** TEST PASSED: DIO_X is LOW when 12V_EN is HIGH ***\r\n");
    } else {
        uart_print("\r\n*** TEST FAILED: DIO_X should be LOW ***\r\n");
    }

    /* Monitor loop */
    uart_print("\r\n--- Monitoring (toggle every 3s) ---\r\n");
    uint32_t count = 0;
    bool en_state = true;

    while (1) {
        /* Toggle 12V_EN */
        en_state = !en_state;
        mcp23017_write_reg(MCP23017_GPIOA, en_state ? GPA_12V_EN : 0x00);
        k_sleep(K_MSEC(100));

        dio_x_val = gpio_pin_get(gpio0_dev, DIO_X_PIN);

        snprintf(buf, sizeof(buf), "[%u] 12V_EN=%s, DIO_X=%s\r\n",
                 count++,
                 en_state ? "HIGH" : "LOW",
                 dio_x_val ? "HIGH" : "LOW");
        uart_print(buf);

        k_sleep(K_SECONDS(3));
    }

    return 0;
}
