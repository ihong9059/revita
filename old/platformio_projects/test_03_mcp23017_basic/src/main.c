/*
 * MCP23017 Test with correct pin configuration
 * 's' = start, 'e' = stop
 */

#include <stdint.h>

// ============ GPIO ============
#define NRF_P0_BASE         0x50000000
#define P0_OUT              (*(volatile uint32_t*)(NRF_P0_BASE + 0x504))
#define P0_OUTSET           (*(volatile uint32_t*)(NRF_P0_BASE + 0x508))
#define P0_OUTCLR           (*(volatile uint32_t*)(NRF_P0_BASE + 0x50C))
#define P0_IN               (*(volatile uint32_t*)(NRF_P0_BASE + 0x510))
#define P0_DIRSET           (*(volatile uint32_t*)(NRF_P0_BASE + 0x518))
#define P0_DIRCLR           (*(volatile uint32_t*)(NRF_P0_BASE + 0x51C))
#define GPIO_PIN_CNF(n)     (*(volatile uint32_t*)(NRF_P0_BASE + 0x700 + (n)*4))

// ============ UART ============
#define NRF_UARTE0_BASE     0x40002000
#define UARTE0_STARTTX      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x008))
#define UARTE0_STARTRX      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x000))
#define UARTE0_STOPTX       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x00C))
#define UARTE0_STOPRX       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x004))
#define UARTE0_ENDTX        (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x120))
#define UARTE0_ENDRX        (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x110))
#define UARTE0_RXDRDY       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x108))
#define UARTE0_ENABLE       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x500))
#define UARTE0_PSEL_RTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x508))
#define UARTE0_PSEL_TXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x50C))
#define UARTE0_PSEL_CTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x510))
#define UARTE0_PSEL_RXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x514))
#define UARTE0_BAUDRATE     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x524))
#define UARTE0_TXD_PTR      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x544))
#define UARTE0_TXD_MAXCNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x548))
#define UARTE0_RXD_PTR      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x534))
#define UARTE0_RXD_MAXCNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x538))
#define UARTE0_RXD_AMOUNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x53C))
#define UARTE0_CONFIG       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x56C))

#define UART_TX_PIN 16
#define UART_RX_PIN 15
#define BAUDRATE_115200 0x01D7E000

// ============ I2C Pins ============
#define I2C_SDA_PIN 13
#define I2C_SCL_PIN 14
#define SDA_MASK (1 << I2C_SDA_PIN)
#define SCL_MASK (1 << I2C_SCL_PIN)

// ============ MCP23017 ============
#define MCP23017_ADDR 0x20
#define MCP_IODIRA   0x00
#define MCP_IODIRB   0x01
#define MCP_IOCON    0x0A
#define MCP_GPPUA    0x0C
#define MCP_GPPUB    0x0D
#define MCP_GPIOA    0x12
#define MCP_GPIOB    0x13
#define MCP_OLATA    0x14
#define MCP_OLATB    0x15

// Pin masks
#define GPA_BTN      0x01
#define GPA_OUT_MASK 0xFE
#define GPB_OUT_MASK 0xFF

#define IODIRA_VALUE 0x01
#define IODIRB_VALUE 0x00

static volatile uint8_t uart_tx_buf[64];
static volatile uint8_t uart_rx_buf[1];

// ============ Delay ============
static void delay_us(uint32_t us) {
    volatile uint32_t count = us * 8;
    while (count--);
}

static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

// ============ UART Functions ============
void uart_init(void) {
    GPIO_PIN_CNF(UART_TX_PIN) = 0x00000003;
    GPIO_PIN_CNF(UART_RX_PIN) = 0x0000000C;
    UARTE0_ENABLE = 0;
    UARTE0_PSEL_TXD = UART_TX_PIN;
    UARTE0_PSEL_RXD = UART_RX_PIN;
    UARTE0_PSEL_RTS = 0xFFFFFFFF;
    UARTE0_PSEL_CTS = 0xFFFFFFFF;
    UARTE0_BAUDRATE = BAUDRATE_115200;
    UARTE0_CONFIG = 0;
    UARTE0_ENABLE = 8;

    // Setup RX
    UARTE0_RXD_PTR = (uint32_t)uart_rx_buf;
    UARTE0_RXD_MAXCNT = 1;
    UARTE0_ENDRX = 0;
    UARTE0_STARTRX = 1;
}

void uart_send(const char* str) {
    int len = 0;
    while (str[len] && len < 63) {
        uart_tx_buf[len] = str[len];
        len++;
    }
    UARTE0_ENDTX = 0;
    UARTE0_TXD_PTR = (uint32_t)uart_tx_buf;
    UARTE0_TXD_MAXCNT = len;
    UARTE0_STARTTX = 1;
    while (UARTE0_ENDTX == 0);
    UARTE0_STOPTX = 1;
}

int uart_rx_ready(void) {
    return UARTE0_ENDRX ? 1 : 0;
}

char uart_rx_char(void) {
    char c = uart_rx_buf[0];
    UARTE0_ENDRX = 0;
    UARTE0_RXD_PTR = (uint32_t)uart_rx_buf;
    UARTE0_RXD_MAXCNT = 1;
    UARTE0_STARTRX = 1;
    return c;
}

void uart_send_hex8(uint8_t val) {
    char hex[5];
    const char* digits = "0123456789ABCDEF";
    hex[0] = '0';
    hex[1] = 'x';
    hex[2] = digits[(val >> 4) & 0x0F];
    hex[3] = digits[val & 0x0F];
    hex[4] = 0;
    uart_send(hex);
}

void uart_send_dec(int val) {
    char buf[12];
    int i = 0;
    if (val == 0) {
        uart_send("0");
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        uart_tx_buf[0] = buf[j];
        UARTE0_ENDTX = 0;
        UARTE0_TXD_PTR = (uint32_t)uart_tx_buf;
        UARTE0_TXD_MAXCNT = 1;
        UARTE0_STARTTX = 1;
        while (UARTE0_ENDTX == 0);
        UARTE0_STOPTX = 1;
    }
}

// ============ Software I2C ============
static void sda_high(void) { P0_DIRCLR = SDA_MASK; }
static void sda_low(void) { P0_OUTCLR = SDA_MASK; P0_DIRSET = SDA_MASK; }
static void scl_high(void) { P0_DIRCLR = SCL_MASK; }
static void scl_low(void) { P0_OUTCLR = SCL_MASK; P0_DIRSET = SCL_MASK; }
static int sda_read(void) { return (P0_IN & SDA_MASK) ? 1 : 0; }

void i2c_init(void) {
    GPIO_PIN_CNF(I2C_SDA_PIN) = (0 << 0) | (0 << 1) | (3 << 2) | (6 << 8);
    GPIO_PIN_CNF(I2C_SCL_PIN) = (0 << 0) | (0 << 1) | (3 << 2) | (6 << 8);
    sda_high();
    scl_high();
    delay_us(10);
}

void i2c_start(void) {
    sda_high(); scl_high(); delay_us(5);
    sda_low(); delay_us(5);
    scl_low(); delay_us(5);
}

void i2c_stop(void) {
    sda_low(); delay_us(5);
    scl_high(); delay_us(5);
    sda_high(); delay_us(5);
}

int i2c_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) sda_high(); else sda_low();
        delay_us(2);
        scl_high(); delay_us(5);
        scl_low(); delay_us(2);
        data <<= 1;
    }
    sda_high(); delay_us(2);
    scl_high(); delay_us(5);
    int ack = sda_read() ? 0 : 1;
    scl_low(); delay_us(2);
    return ack;
}

uint8_t i2c_read_byte(int ack) {
    uint8_t data = 0;
    sda_high();
    for (int i = 0; i < 8; i++) {
        data <<= 1;
        scl_high(); delay_us(5);
        if (sda_read()) data |= 1;
        scl_low(); delay_us(5);
    }
    if (ack) sda_low(); else sda_high();
    delay_us(2);
    scl_high(); delay_us(5);
    scl_low(); delay_us(2);
    sda_high();
    return data;
}

// ============ MCP23017 Functions ============
void mcp_write_reg(uint8_t reg, uint8_t val) {
    i2c_start();
    i2c_write_byte(MCP23017_ADDR << 1);
    i2c_write_byte(reg);
    i2c_write_byte(val);
    i2c_stop();
}

uint8_t mcp_read_reg(uint8_t reg) {
    i2c_start();
    i2c_write_byte(MCP23017_ADDR << 1);
    i2c_write_byte(reg);
    i2c_start();
    i2c_write_byte((MCP23017_ADDR << 1) | 1);
    uint8_t val = i2c_read_byte(0);
    i2c_stop();
    return val;
}

// ============ Main ============
int main(void) {
    uart_init();
    delay_ms(100);

    uart_send("\r\n=== MCP23017 All LOW Setting ===\r\n");

    i2c_init();
    delay_ms(100);

    // Set IOCON: BANK=0, SEQOP=1
    mcp_write_reg(MCP_IOCON, 0x20);
    delay_ms(10);

    // Configure GPIOA and GPIOB as all outputs
    mcp_write_reg(MCP_IODIRA, 0x00);
    mcp_write_reg(MCP_IODIRB, 0x00);
    delay_ms(10);

    // Set all outputs to LOW
    mcp_write_reg(MCP_GPIOA, 0x00);
    mcp_write_reg(MCP_GPIOB, 0x00);
    delay_ms(10);

    // Read and display status
    uart_send("IODIRA=");
    uart_send_hex8(mcp_read_reg(MCP_IODIRA));
    uart_send(" GPIOA=");
    uart_send_hex8(mcp_read_reg(MCP_GPIOA));
    uart_send("\r\n");

    uart_send("IODIRB=");
    uart_send_hex8(mcp_read_reg(MCP_IODIRB));
    uart_send(" GPIOB=");
    uart_send_hex8(mcp_read_reg(MCP_GPIOB));
    uart_send("\r\n");

    uart_send("\r\nAll outputs set to LOW.\r\n");

    while (1) {
        delay_ms(1000);
    }

    return 0;
}

__attribute__((section(".vectors")))
const uint32_t vectors[] = {
    0x20040000,
    (uint32_t)main,
};
