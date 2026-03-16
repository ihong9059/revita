/*
 * Radio TX - nRF52840 Bare Metal
 */

#include <stdint.h>

#define NRF_P0_BASE         0x50000000
#define P0_PIN_CNF(n)       (*(volatile uint32_t*)(NRF_P0_BASE + 0x700 + (n)*4))

#define NRF_CLOCK_BASE      0x40000000
#define CLOCK_TASKS_HFCLKSTART  (*(volatile uint32_t*)(NRF_CLOCK_BASE + 0x000))
#define CLOCK_EVENTS_HFCLKSTARTED (*(volatile uint32_t*)(NRF_CLOCK_BASE + 0x100))

#define NRF_RADIO_BASE      0x40001000
#define RADIO_TASKS_TXEN        (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x000))
#define RADIO_TASKS_START       (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x008))
#define RADIO_TASKS_DISABLE     (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x010))
#define RADIO_EVENTS_READY      (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x100))
#define RADIO_EVENTS_END        (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x10C))
#define RADIO_EVENTS_DISABLED   (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x110))
#define RADIO_PACKETPTR         (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x504))
#define RADIO_FREQUENCY         (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x508))
#define RADIO_TXPOWER           (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x50C))
#define RADIO_MODE              (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x510))
#define RADIO_PCNF0             (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x514))
#define RADIO_PCNF1             (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x518))
#define RADIO_BASE0             (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x51C))
#define RADIO_PREFIX0           (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x524))
#define RADIO_TXADDRESS         (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x52C))
#define RADIO_CRCCNF            (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x534))
#define RADIO_CRCPOLY           (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x538))
#define RADIO_CRCINIT           (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x53C))
#define RADIO_STATE             (*(volatile uint32_t*)(NRF_RADIO_BASE + 0x550))

#define NRF_UARTE0_BASE     0x40002000
#define UARTE0_STARTTX      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x008))
#define UARTE0_ENDTX        (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x120))
#define UARTE0_ENABLE       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x500))
#define UARTE0_PSEL_TXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x50C))
#define UARTE0_PSEL_RXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x514))
#define UARTE0_PSEL_RTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x508))
#define UARTE0_PSEL_CTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x510))
#define UARTE0_BAUDRATE     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x524))
#define UARTE0_TXD_PTR      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x544))
#define UARTE0_TXD_MAXCNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x548))

static uint8_t tx_packet[32] __attribute__((aligned(4)));
static char tx_buf[64];

void uart_init(void) {
    P0_PIN_CNF(16) = 3;
    P0_PIN_CNF(15) = 0;
    UARTE0_PSEL_TXD = 16;
    UARTE0_PSEL_RXD = 15;
    UARTE0_PSEL_RTS = 0xFFFFFFFF;
    UARTE0_PSEL_CTS = 0xFFFFFFFF;
    UARTE0_BAUDRATE = 0x01D7E000;
    UARTE0_ENABLE = 8;
}

void uart_print(const char* s) {
    uint32_t len = 0;
    while (s[len] && len < 60) { tx_buf[len] = s[len]; len++; }
    if (!len) return;
    UARTE0_TXD_PTR = (uint32_t)tx_buf;
    UARTE0_TXD_MAXCNT = len;
    UARTE0_ENDTX = 0;
    UARTE0_STARTTX = 1;
    while (!UARTE0_ENDTX);
}

void uart_dec(uint32_t v) {
    char b[12]; int i = 10; b[11] = 0;
    if (!v) { uart_print("0"); return; }
    while (v && i >= 0) { b[i--] = '0' + (v % 10); v /= 10; }
    uart_print(&b[i+1]);
}

void hfclk_start(void) {
    CLOCK_EVENTS_HFCLKSTARTED = 0;
    CLOCK_TASKS_HFCLKSTART = 1;
    while (!CLOCK_EVENTS_HFCLKSTARTED);
}

void radio_init(void) {
    RADIO_TASKS_DISABLE = 1;
    while (RADIO_STATE != 0);

    RADIO_MODE = 0;           // NRF_1MBIT
    RADIO_FREQUENCY = 80;      // 2407 MHz
    RADIO_TXPOWER = 8;        // +8 dBm (max)

    // Packet config: 8-bit length field
    RADIO_PCNF0 = (8 << 0);

    // MAXLEN=32, STATLEN=0, BALEN=3 (3 bytes from BASE0[31:8])
    RADIO_PCNF1 = (32 << 0) | (0 << 8) | (3 << 16);

    // Address: 0xE7E7E7E7 (standard ShockBurst)
    RADIO_BASE0 = 0xE7E7E700;   // bits[31:8] = E7 E7 E7
    RADIO_PREFIX0 = 0xE7;
    RADIO_TXADDRESS = 0;

    // CRC: 2 bytes
    RADIO_CRCCNF = (2 << 0) | (1 << 8);  // LEN=2, SKIPADDR=1
    RADIO_CRCPOLY = 0x11021;
    RADIO_CRCINIT = 0xFFFF;

    RADIO_PACKETPTR = (uint32_t)tx_packet;
}

void radio_tx(void) {
    RADIO_EVENTS_READY = 0;
    RADIO_EVENTS_END = 0;
    RADIO_EVENTS_DISABLED = 0;

    RADIO_TASKS_TXEN = 1;
    while (!RADIO_EVENTS_READY);

    RADIO_TASKS_START = 1;
    while (!RADIO_EVENTS_END);

    RADIO_TASKS_DISABLE = 1;
    while (!RADIO_EVENTS_DISABLED);
}

int main(void) {
    uint8_t cnt = 0;

    hfclk_start();
    uart_init();

    for (volatile int i = 0; i < 1000000; i++);

    uart_print("\r\n\r\n== TX ==\r\n");

    radio_init();

    while (1) {
        // Packet: [LEN=4][0xAA][0xBB][cnt][0xDD]
        tx_packet[0] = 4;
        tx_packet[1] = 0xAA;
        tx_packet[2] = 0xBB;
        tx_packet[3] = cnt;
        tx_packet[4] = 0xDD;

        radio_tx();

        uart_print("T:");
        uart_dec(cnt);
        uart_print("\r\n");

        cnt++;

        // 500ms delay
        for (volatile uint32_t i = 0; i < 32000000; i++);
    }
    return 0;
}

__attribute__((section(".vectors")))
const uint32_t vectors[] = { 0x20040000, (uint32_t)main };
