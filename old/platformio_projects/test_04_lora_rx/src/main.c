/*
 * LoRa RX - SX1262 Receiver (RAK4630)
 * Receives packets and displays counter value
 */

#include <stdint.h>

// ============ GPIO (P0 and P1) ============
#define NRF_P0_BASE         0x50000000
#define NRF_P1_BASE         0x50000300

#define P0_OUT              (*(volatile uint32_t*)(NRF_P0_BASE + 0x504))
#define P0_OUTSET           (*(volatile uint32_t*)(NRF_P0_BASE + 0x508))
#define P0_OUTCLR           (*(volatile uint32_t*)(NRF_P0_BASE + 0x50C))
#define P0_IN               (*(volatile uint32_t*)(NRF_P0_BASE + 0x510))
#define P0_DIRSET           (*(volatile uint32_t*)(NRF_P0_BASE + 0x518))
#define P0_DIRCLR           (*(volatile uint32_t*)(NRF_P0_BASE + 0x51C))
#define P0_PIN_CNF(n)       (*(volatile uint32_t*)(NRF_P0_BASE + 0x700 + (n)*4))

#define P1_OUT              (*(volatile uint32_t*)(NRF_P1_BASE + 0x504))
#define P1_OUTSET           (*(volatile uint32_t*)(NRF_P1_BASE + 0x508))
#define P1_OUTCLR           (*(volatile uint32_t*)(NRF_P1_BASE + 0x50C))
#define P1_IN               (*(volatile uint32_t*)(NRF_P1_BASE + 0x510))
#define P1_DIRSET           (*(volatile uint32_t*)(NRF_P1_BASE + 0x518))
#define P1_DIRCLR           (*(volatile uint32_t*)(NRF_P1_BASE + 0x51C))
#define P1_PIN_CNF(n)       (*(volatile uint32_t*)(NRF_P1_BASE + 0x700 + (n)*4))

// ============ SPI (SPIM3) ============
#define NRF_SPIM3_BASE      0x4002F000
#define SPIM3_TASKS_START   (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x010))
#define SPIM3_TASKS_STOP    (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x014))
#define SPIM3_EVENTS_END    (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x118))
#define SPIM3_ENABLE        (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x500))
#define SPIM3_PSEL_SCK      (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x508))
#define SPIM3_PSEL_MOSI     (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x50C))
#define SPIM3_PSEL_MISO     (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x510))
#define SPIM3_FREQUENCY     (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x524))
#define SPIM3_CONFIG        (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x554))
#define SPIM3_TXD_PTR       (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x544))
#define SPIM3_TXD_MAXCNT    (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x548))
#define SPIM3_RXD_PTR       (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x534))
#define SPIM3_RXD_MAXCNT    (*(volatile uint32_t*)(NRF_SPIM3_BASE + 0x538))

// ============ UART ============
#define NRF_UARTE0_BASE     0x40002000
#define UARTE0_STARTTX      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x008))
#define UARTE0_STOPTX       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x00C))
#define UARTE0_ENDTX        (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x120))
#define UARTE0_ENABLE       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x500))
#define UARTE0_PSEL_TXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x50C))
#define UARTE0_PSEL_RXD     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x514))
#define UARTE0_PSEL_RTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x508))
#define UARTE0_PSEL_CTS     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x510))
#define UARTE0_BAUDRATE     (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x524))
#define UARTE0_TXD_PTR      (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x544))
#define UARTE0_TXD_MAXCNT   (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x548))
#define UARTE0_CONFIG       (*(volatile uint32_t*)(NRF_UARTE0_BASE + 0x56C))

#define UART_TX_PIN 16
#define UART_RX_PIN 15
#define BAUDRATE_115200 0x01D7E000

// ============ SX1262 Pins (RAK4630) ============
#define SX_NSS_PIN      10  // P1.10
#define SX_SCK_PIN      11  // P1.11
#define SX_MOSI_PIN     12  // P1.12
#define SX_MISO_PIN     13  // P1.13
#define SX_RESET_PIN    6   // P1.06
#define SX_DIO1_PIN     15  // P1.15
#define SX_BUSY_PIN     14  // P1.14
#define SX_ANT_SW_PIN   5   // P1.05

// ============ SX1262 Commands ============
#define SX_CMD_SET_SLEEP            0x84
#define SX_CMD_SET_STANDBY          0x80
#define SX_CMD_SET_FS               0xC1
#define SX_CMD_SET_TX               0x83
#define SX_CMD_SET_RX               0x82
#define SX_CMD_SET_PACKET_TYPE      0x8A
#define SX_CMD_SET_RF_FREQUENCY     0x86
#define SX_CMD_SET_PA_CONFIG        0x95
#define SX_CMD_SET_TX_PARAMS        0x8E
#define SX_CMD_SET_MODULATION_PARAMS 0x8B
#define SX_CMD_SET_PACKET_PARAMS    0x8C
#define SX_CMD_SET_DIO_IRQ_PARAMS   0x08
#define SX_CMD_SET_DIO2_AS_RF_SW    0x9D
#define SX_CMD_SET_DIO3_AS_TCXO     0x97
#define SX_CMD_CALIBRATE            0x89
#define SX_CMD_CALIBRATE_IMAGE      0x98
#define SX_CMD_SET_REGULATOR_MODE   0x96
#define SX_CMD_SET_BUFFER_BASE_ADDR 0x8F
#define SX_CMD_WRITE_BUFFER         0x0E
#define SX_CMD_READ_BUFFER          0x1E
#define SX_CMD_GET_STATUS           0xC0
#define SX_CMD_GET_IRQ_STATUS       0x12
#define SX_CMD_CLR_IRQ_STATUS       0x02
#define SX_CMD_GET_RX_BUFFER_STATUS 0x13
#define SX_CMD_GET_PACKET_STATUS    0x14

// LoRa parameters (must match TX)
#define LORA_FREQUENCY      923000000  // 923 MHz
#define LORA_SF             7          // Spreading Factor
#define LORA_BW             0x04       // 125 kHz
#define LORA_CR             0x01       // 4/5
#define LORA_PREAMBLE_LEN   8

static volatile uint8_t uart_tx_buf[128];
static volatile uint8_t spi_tx_buf[16];
static volatile uint8_t spi_rx_buf[16];

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
    P0_PIN_CNF(UART_TX_PIN) = 0x00000001;
    P0_PIN_CNF(UART_RX_PIN) = 0x00000000;
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
    while (str[len] && len < 127) {
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
    if (val < 0) {
        uart_send("-");
        val = -val;
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

// ============ SPI Functions ============
void spi_init(void) {
    // Configure NSS as output, high
    P1_PIN_CNF(SX_NSS_PIN) = 0x00000001;
    P1_OUTSET = (1 << SX_NSS_PIN);
    P1_DIRSET = (1 << SX_NSS_PIN);

    // Configure SPI pins
    P1_PIN_CNF(SX_SCK_PIN) = 0x00000001;
    P1_PIN_CNF(SX_MOSI_PIN) = 0x00000001;
    P1_PIN_CNF(SX_MISO_PIN) = 0x00000000;

    // Configure control pins
    P1_PIN_CNF(SX_RESET_PIN) = 0x00000001;
    P1_OUTSET = (1 << SX_RESET_PIN);
    P1_DIRSET = (1 << SX_RESET_PIN);

    P1_PIN_CNF(SX_BUSY_PIN) = 0x00000000;  // Input
    P1_PIN_CNF(SX_DIO1_PIN) = 0x00000000;  // Input

    P1_PIN_CNF(SX_ANT_SW_PIN) = 0x00000001;
    P1_OUTCLR = (1 << SX_ANT_SW_PIN);
    P1_DIRSET = (1 << SX_ANT_SW_PIN);

    // Configure SPIM3
    SPIM3_ENABLE = 0;
    SPIM3_PSEL_SCK = (1 << 5) | SX_SCK_PIN;   // Port 1
    SPIM3_PSEL_MOSI = (1 << 5) | SX_MOSI_PIN;
    SPIM3_PSEL_MISO = (1 << 5) | SX_MISO_PIN;
    SPIM3_FREQUENCY = 0x02000000;  // 1 MHz
    SPIM3_CONFIG = 0;  // Mode 0
    SPIM3_ENABLE = 7;
}

void sx_nss_low(void) { P1_OUTCLR = (1 << SX_NSS_PIN); }
void sx_nss_high(void) { P1_OUTSET = (1 << SX_NSS_PIN); }

void sx_wait_busy(void) {
    while (P1_IN & (1 << SX_BUSY_PIN)) {
        delay_us(1);
    }
}

void sx_reset(void) {
    P1_OUTCLR = (1 << SX_RESET_PIN);
    delay_ms(20);
    P1_OUTSET = (1 << SX_RESET_PIN);
    delay_ms(10);
    sx_wait_busy();
}

void spi_transfer(uint8_t* tx, uint8_t* rx, uint8_t len) {
    for (int i = 0; i < len; i++) {
        spi_tx_buf[i] = tx ? tx[i] : 0;
    }

    SPIM3_TXD_PTR = (uint32_t)spi_tx_buf;
    SPIM3_TXD_MAXCNT = len;
    SPIM3_RXD_PTR = (uint32_t)spi_rx_buf;
    SPIM3_RXD_MAXCNT = len;

    SPIM3_EVENTS_END = 0;
    SPIM3_TASKS_START = 1;
    while (SPIM3_EVENTS_END == 0);

    if (rx) {
        for (int i = 0; i < len; i++) {
            rx[i] = spi_rx_buf[i];
        }
    }
}

void sx_write_command(uint8_t cmd, uint8_t* data, uint8_t len) {
    sx_wait_busy();
    sx_nss_low();
    spi_transfer(&cmd, 0, 1);
    if (len > 0 && data) {
        spi_transfer(data, 0, len);
    }
    sx_nss_high();
}

void sx_read_command(uint8_t cmd, uint8_t* data, uint8_t len) {
    sx_wait_busy();
    sx_nss_low();
    spi_transfer(&cmd, 0, 1);
    uint8_t nop = 0;
    spi_transfer(&nop, 0, 1);  // NOP for status
    if (len > 0 && data) {
        for (int i = 0; i < len; i++) {
            spi_transfer(&nop, &data[i], 1);
        }
    }
    sx_nss_high();
}

void sx_read_buffer(uint8_t offset, uint8_t* data, uint8_t len) {
    sx_wait_busy();
    sx_nss_low();
    uint8_t cmd = SX_CMD_READ_BUFFER;
    spi_transfer(&cmd, 0, 1);
    spi_transfer(&offset, 0, 1);
    uint8_t nop = 0;
    spi_transfer(&nop, 0, 1);  // NOP
    for (int i = 0; i < len; i++) {
        spi_transfer(&nop, &data[i], 1);
    }
    sx_nss_high();
}

// ============ SX1262 Configuration ============
void sx_set_standby(void) {
    uint8_t data = 0x00;  // STDBY_RC
    sx_write_command(SX_CMD_SET_STANDBY, &data, 1);
}

void sx_set_packet_type(void) {
    uint8_t data = 0x01;  // LoRa
    sx_write_command(SX_CMD_SET_PACKET_TYPE, &data, 1);
}

void sx_set_rf_frequency(uint32_t freq) {
    uint32_t frf = (uint32_t)((double)freq / (double)32000000 * (double)(1 << 25));
    uint8_t data[4];
    data[0] = (frf >> 24) & 0xFF;
    data[1] = (frf >> 16) & 0xFF;
    data[2] = (frf >> 8) & 0xFF;
    data[3] = frf & 0xFF;
    sx_write_command(SX_CMD_SET_RF_FREQUENCY, data, 4);
}

void sx_set_modulation_params(void) {
    uint8_t data[4];
    data[0] = LORA_SF;   // SF
    data[1] = LORA_BW;   // BW
    data[2] = LORA_CR;   // CR
    data[3] = 0x00;      // LowDataRateOptimize (off)
    sx_write_command(SX_CMD_SET_MODULATION_PARAMS, data, 4);
}

void sx_set_packet_params(uint8_t payload_len) {
    uint8_t data[6];
    data[0] = (LORA_PREAMBLE_LEN >> 8) & 0xFF;
    data[1] = LORA_PREAMBLE_LEN & 0xFF;
    data[2] = 0x00;  // Header type: explicit
    data[3] = payload_len;
    data[4] = 0x01;  // CRC on
    data[5] = 0x00;  // Standard IQ
    sx_write_command(SX_CMD_SET_PACKET_PARAMS, data, 6);
}

void sx_set_dio_irq_params(void) {
    uint8_t data[8];
    uint16_t irq_mask = 0x0002;  // RxDone
    data[0] = (irq_mask >> 8) & 0xFF;
    data[1] = irq_mask & 0xFF;
    data[2] = (irq_mask >> 8) & 0xFF;  // DIO1
    data[3] = irq_mask & 0xFF;
    data[4] = 0x00;  // DIO2
    data[5] = 0x00;
    data[6] = 0x00;  // DIO3
    data[7] = 0x00;
    sx_write_command(SX_CMD_SET_DIO_IRQ_PARAMS, data, 8);
}

void sx_set_dio2_as_rf_switch(void) {
    uint8_t data = 0x01;
    sx_write_command(SX_CMD_SET_DIO2_AS_RF_SW, &data, 1);
}

void sx_set_dio3_as_tcxo(void) {
    uint8_t data[4];
    data[0] = 0x07;  // TCXO voltage 3.3V
    data[1] = 0x00;  // Timeout
    data[2] = 0x01;
    data[3] = 0x40;  // 5ms
    sx_write_command(SX_CMD_SET_DIO3_AS_TCXO, data, 4);
}

void sx_set_regulator_mode(void) {
    uint8_t data = 0x01;  // DC-DC
    sx_write_command(SX_CMD_SET_REGULATOR_MODE, &data, 1);
}

void sx_calibrate(void) {
    uint8_t data = 0x7F;  // All calibrations
    sx_write_command(SX_CMD_CALIBRATE, &data, 1);
    delay_ms(10);
}

void sx_calibrate_image(void) {
    uint8_t data[2];
    data[0] = 0xE1;  // 902-928 MHz
    data[1] = 0xE9;
    sx_write_command(SX_CMD_CALIBRATE_IMAGE, data, 2);
}

void sx_set_buffer_base_addr(void) {
    uint8_t data[2];
    data[0] = 0x00;  // TX base
    data[1] = 0x00;  // RX base
    sx_write_command(SX_CMD_SET_BUFFER_BASE_ADDR, data, 2);
}

void sx_clear_irq_status(void) {
    uint8_t data[2] = {0xFF, 0xFF};
    sx_write_command(SX_CMD_CLR_IRQ_STATUS, data, 2);
}

void sx_set_rx(uint32_t timeout) {
    uint8_t data[3];
    data[0] = (timeout >> 16) & 0xFF;
    data[1] = (timeout >> 8) & 0xFF;
    data[2] = timeout & 0xFF;
    sx_write_command(SX_CMD_SET_RX, data, 3);
}

uint16_t sx_get_irq_status(void) {
    uint8_t data[2];
    sx_read_command(SX_CMD_GET_IRQ_STATUS, data, 2);
    return (data[0] << 8) | data[1];
}

void sx_get_rx_buffer_status(uint8_t* payload_len, uint8_t* rx_start) {
    uint8_t data[2];
    sx_read_command(SX_CMD_GET_RX_BUFFER_STATUS, data, 2);
    *payload_len = data[0];
    *rx_start = data[1];
}

void sx_get_packet_status(int8_t* rssi, int8_t* snr) {
    uint8_t data[3];
    sx_read_command(SX_CMD_GET_PACKET_STATUS, data, 3);
    *rssi = -data[0] / 2;
    *snr = (int8_t)data[1] / 4;
}

// ============ SX1262 Init ============
void sx_init(void) {
    uart_send("SX1262 Init...\r\n");

    sx_reset();
    uart_send("  Reset done\r\n");

    sx_set_standby();
    uart_send("  Standby\r\n");

    sx_set_dio3_as_tcxo();
    delay_ms(5);

    sx_calibrate();
    uart_send("  Calibrated\r\n");

    sx_set_dio2_as_rf_switch();
    sx_set_regulator_mode();

    sx_set_packet_type();
    sx_set_rf_frequency(LORA_FREQUENCY);
    uart_send("  Freq: 923MHz\r\n");

    sx_set_modulation_params();
    sx_calibrate_image();

    sx_set_buffer_base_addr();
    sx_set_packet_params(255);  // Max payload
    sx_set_dio_irq_params();

    uart_send("  Init complete\r\n");
}

// ============ Start RX ============
void sx_start_rx(void) {
    sx_set_standby();
    sx_clear_irq_status();

    // Enable antenna for RX
    P1_OUTSET = (1 << SX_ANT_SW_PIN);

    sx_set_rx(0xFFFFFF);  // Continuous RX
}

// ============ Main ============
int main(void) {
    uart_init();
    delay_ms(100);

    uart_send("\r\n=== LoRa RX (SX1262) ===\r\n");

    spi_init();
    delay_ms(100);

    sx_init();

    uart_send("\r\nListening...\r\n");
    sx_start_rx();

    uint8_t rx_buf[64];

    while (1) {
        uint16_t irq = sx_get_irq_status();

        if (irq & 0x0002) {  // RxDone
            sx_clear_irq_status();

            uint8_t payload_len, rx_start;
            sx_get_rx_buffer_status(&payload_len, &rx_start);

            int8_t rssi, snr;
            sx_get_packet_status(&rssi, &snr);

            sx_read_buffer(rx_start, rx_buf, payload_len);

            uart_send("RX [");
            uart_send_dec(rx_buf[3]);  // Counter value
            uart_send("] RSSI=");
            uart_send_dec(rssi);
            uart_send(" SNR=");
            uart_send_dec(snr);
            uart_send(" Len=");
            uart_send_dec(payload_len);
            uart_send("\r\n");

            // Restart RX
            sx_start_rx();
        }

        delay_ms(10);
    }

    return 0;
}

__attribute__((section(".vectors")))
const uint32_t vectors[] = {
    0x20040000,
    (uint32_t)main,
};
