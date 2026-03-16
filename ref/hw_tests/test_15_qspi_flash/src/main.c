/*
 * QSPI Flash Test for nRF52840
 * Reads JEDEC ID and flash data
 */

#include <stdint.h>

// ============ GPIO ============
#define NRF_P0_BASE         0x50000000
#define P0_PIN_CNF(n)       (*(volatile uint32_t*)(NRF_P0_BASE + 0x700 + (n)*4))

// ============ UART ============
#define NRF_UARTE0_BASE     0x40002000

#define UARTE0_STARTTX      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x008))
#define UARTE0_STOPTX       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x00C))
#define UARTE0_ENDTX        (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x120))
#define UARTE0_ENABLE       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x500))
#define UARTE0_PSEL_RTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x508))
#define UARTE0_PSEL_TXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x50C))
#define UARTE0_PSEL_CTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x510))
#define UARTE0_PSEL_RXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x514))
#define UARTE0_BAUDRATE     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x524))
#define UARTE0_TXD_PTR      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x544))
#define UARTE0_TXD_MAXCNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x548))
#define UARTE0_CONFIG       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x56C))

// ============ QSPI ============
#define NRF_QSPI_BASE       0x40029000

#define QSPI_ACTIVATE       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x000))
#define QSPI_READSTART      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x004))
#define QSPI_WRITESTART     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x008))
#define QSPI_ERASESTART     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x00C))
#define QSPI_DEACTIVATE     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x010))
#define QSPI_READY          (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x100))
#define QSPI_ENABLE         (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x500))
#define QSPI_READ_SRC       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x504))
#define QSPI_READ_DST       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x508))
#define QSPI_READ_CNT       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x50C))
#define QSPI_WRITE_DST      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x510))
#define QSPI_WRITE_SRC      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x514))
#define QSPI_WRITE_CNT      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x518))
#define QSPI_ERASE_PTR      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x51C))
#define QSPI_ERASE_LEN      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x520))
#define QSPI_PSEL_SCK       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x524))
#define QSPI_PSEL_CSN       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x528))
#define QSPI_PSEL_IO0       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x530))
#define QSPI_PSEL_IO1       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x534))
#define QSPI_PSEL_IO2       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x538))
#define QSPI_PSEL_IO3       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x53C))
#define QSPI_XIPOFFSET      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x540))
#define QSPI_IFCONFIG0      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x544))
#define QSPI_IFCONFIG1      (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x600))
#define QSPI_STATUS         (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x604))
#define QSPI_DPMDUR         (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x614))
#define QSPI_ADDRCONF       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x624))
#define QSPI_CINSTRCONF     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x634))
#define QSPI_CINSTRDAT0     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x638))
#define QSPI_CINSTRDAT1     (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x63C))
#define QSPI_IFTIMING       (*(volatile uint32_t*)(NRF_QSPI_BASE + 0x640))

// QSPI Pins for REVITA_LINK (MX25R1635F external flash)
// Based on REVITA_LINK_v1 schematic
#define QSPI_SCK_PIN    3    // P0.03
#define QSPI_CSN_PIN    26   // P0.26
#define QSPI_IO0_PIN    30   // P0.30
#define QSPI_IO1_PIN    29   // P0.29
#define QSPI_IO2_PIN    28   // P0.28
#define QSPI_IO3_PIN    2    // P0.02

// Flash commands
#define CMD_RDID        0x9F    // Read JEDEC ID
#define CMD_READ        0x03    // Read data
#define CMD_WREN        0x06    // Write enable
#define CMD_RDSR        0x05    // Read status register

#define UART_TX_PIN     16
#define UART_RX_PIN     15
#define BAUDRATE_115200 0x01D7E000

static volatile uint8_t uart_tx_buf[64];
static volatile uint8_t flash_buf[256] __attribute__((aligned(4)));

// ============ Delay ============
static void delay_us(uint32_t us) {
    volatile uint32_t count = us * 8;
    while (count--);
}
static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

// ============ UART ============
void uart_init(void) {
    // TX: Output, INPUT=Disconnect (0x3)
    // RX: Input, Pull-up (0xC)
    P0_PIN_CNF(UART_TX_PIN) = 0x00000003;
    P0_PIN_CNF(UART_RX_PIN) = 0x0000000C;
    UARTE0_ENABLE = 0;
    UARTE0_PSEL_TXD = UART_TX_PIN;
    UARTE0_PSEL_RXD = UART_RX_PIN;
    UARTE0_PSEL_RTS = 0xFFFFFFFF;
    UARTE0_PSEL_CTS = 0xFFFFFFFF;
    UARTE0_BAUDRATE = BAUDRATE_115200;
    UARTE0_CONFIG = 0;
    UARTE0_ENABLE = 8;
}

void uart_send(const char* str) {
    int len = 0;
    while (str[len] && len < 63) { uart_tx_buf[len] = str[len]; len++; }
    UARTE0_ENDTX = 0;
    UARTE0_TXD_PTR = (uint32_t)uart_tx_buf;
    UARTE0_TXD_MAXCNT = len;
    UARTE0_STARTTX = 1;
    while (UARTE0_ENDTX == 0);
    UARTE0_STOPTX = 1;
}

void uart_hex8(uint8_t v) {
    const char* d = "0123456789ABCDEF";
    char h[5] = {'0','x', d[(v>>4)&0xF], d[v&0xF], 0};
    uart_send(h);
}

void uart_hex32(uint32_t v) {
    const char* d = "0123456789ABCDEF";
    char h[11];
    h[0] = '0'; h[1] = 'x';
    for (int i = 0; i < 8; i++) {
        h[2+i] = d[(v >> (28 - i*4)) & 0xF];
    }
    h[10] = 0;
    uart_send(h);
}

// ============ QSPI ============
void qspi_init(void) {
    // Disable first
    QSPI_ENABLE = 0;
    delay_ms(10);

    // Configure pins
    QSPI_PSEL_SCK = QSPI_SCK_PIN;
    QSPI_PSEL_CSN = QSPI_CSN_PIN;
    QSPI_PSEL_IO0 = QSPI_IO0_PIN;
    QSPI_PSEL_IO1 = QSPI_IO1_PIN;
    QSPI_PSEL_IO2 = QSPI_IO2_PIN;
    QSPI_PSEL_IO3 = QSPI_IO3_PIN;

    // IFCONFIG0: READOC=0 (FASTREAD), WRITEOC=0, ADDRMODE=0 (24-bit), DPMENABLE=0
    QSPI_IFCONFIG0 = 0;

    // IFCONFIG1: SCKDELAY=128, SPIMODE=0, SCKFREQ=15 (2MHz)
    QSPI_IFCONFIG1 = (128 << 0) | (0 << 25) | (15 << 28);

    // Enable QSPI
    QSPI_ENABLE = 1;
    delay_ms(1);

    // Activate
    QSPI_READY = 0;
    QSPI_ACTIVATE = 1;
    while (QSPI_READY == 0);
}

void qspi_custom_cmd(uint8_t cmd, int wlen, int rlen) {
    // CINSTRCONF: OPCODE, LENGTH, LIO2, LIO3, WIPWAIT, WREN
    // LENGTH = wlen + rlen + 1 (for opcode)
    uint32_t len = 1 + wlen + rlen;
    if (len > 9) len = 9;
    QSPI_CINSTRCONF = (cmd << 0) | (len << 8) | (0 << 12) | (0 << 13) | (0 << 14) | (0 << 15);

    QSPI_READY = 0;
    // Trigger by writing to CINSTRCONF (already done)
    // Need to wait for READY
    volatile int timeout = 100000;
    while (QSPI_READY == 0 && timeout-- > 0);
}

uint32_t qspi_read_jedec_id(void) {
    QSPI_CINSTRDAT0 = 0;
    QSPI_CINSTRDAT1 = 0;

    // RDID command: 1 byte opcode, 3 bytes response
    // LENGTH = 4, LIO2=0, LIO3=0
    QSPI_CINSTRCONF = (CMD_RDID << 0) | (4 << 8);

    QSPI_READY = 0;
    volatile int timeout = 100000;
    while (QSPI_READY == 0 && timeout-- > 0);

    return QSPI_CINSTRDAT0 & 0x00FFFFFF;
}

int qspi_read_data(uint32_t addr, uint8_t* buf, int len) {
    if (len > 256) len = 256;

    QSPI_READ_SRC = addr;
    QSPI_READ_DST = (uint32_t)buf;
    QSPI_READ_CNT = len;

    QSPI_READY = 0;
    QSPI_READSTART = 1;

    volatile int timeout = 1000000;
    while (QSPI_READY == 0 && timeout-- > 0);

    return (timeout > 0) ? len : -1;
}

// ============ Main ============
int main(void) {
    uart_init();
    delay_ms(100);

    uart_send("\r\n");
    uart_send("========================================\r\n");
    uart_send("  QSPI Flash Test\r\n");
    uart_send("========================================\r\n");
    uart_send("\r\n");

    uart_send("Initializing QSPI...\r\n");
    qspi_init();
    uart_send("QSPI initialized.\r\n\r\n");

    // Read JEDEC ID
    uart_send("Reading JEDEC ID...\r\n");
    uint32_t jedec_id = qspi_read_jedec_id();
    uart_send("  JEDEC ID: ");
    uart_hex32(jedec_id);
    uart_send("\r\n");

    // Decode manufacturer
    uint8_t mfr = jedec_id & 0xFF;
    uart_send("  Manufacturer: ");
    if (mfr == 0xC2) uart_send("Macronix");
    else if (mfr == 0xEF) uart_send("Winbond");
    else if (mfr == 0x20) uart_send("Micron");
    else if (mfr == 0x01) uart_send("Spansion");
    else if (mfr == 0xBF) uart_send("SST");
    else if (mfr == 0x1F) uart_send("Adesto");
    else { uart_send("Unknown ("); uart_hex8(mfr); uart_send(")"); }
    uart_send("\r\n\r\n");

    // Read first 64 bytes
    uart_send("Reading first 64 bytes from address 0x00000000:\r\n");
    int ret = qspi_read_data(0x00000000, (uint8_t*)flash_buf, 64);
    if (ret > 0) {
        for (int i = 0; i < 64; i++) {
            if (i % 16 == 0) {
                uart_send("\r\n  ");
                uart_hex32(i);
                uart_send(": ");
            }
            uart_hex8(flash_buf[i]);
            uart_send(" ");
        }
        uart_send("\r\n");
    } else {
        uart_send("  Read failed!\r\n");
    }

    uart_send("\r\nQSPI Flash test complete.\r\n");

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
