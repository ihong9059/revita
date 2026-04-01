#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include <stdbool.h>

/*
 * LoRa Star Topology - End Device (RX)
 *
 * Frame Structure (16 bytes over-the-air):
 * ┌─────────────────────────────────────────────────────┐
 * │ LoRa PHY: Preamble(8sym) + Header + FEC(CR4/6)     │
 * │ ┌─────────────────────────────────────────────────┐ │
 * │ │ Encrypted Application Payload (16 bytes)        │ │
 * │ │  [0]    Magic: 0xAA                             │ │
 * │ │  [1]    Type:  0x01 (count broadcast)           │ │
 * │ │  [2]    Seq:   0-255                            │ │
 * │ │  [3]    Flags: 0x01 (encrypted)                 │ │
 * │ │  [4-7]  Count (uint32 BE)                       │ │
 * │ │  [8-11] Uptime seconds (uint32 BE)              │ │
 * │ │  [12-13] CRC-16 (bytes 0-11)                    │ │
 * │ │  [14-15] Padding 0x0000                         │ │
 * │ └─────────────────────────────────────────────────┘ │
 * │ LoRa PHY CRC (2 bytes, HW generated)               │
 * └─────────────────────────────────────────────────────┘
 *
 * Encryption: AES-128-CTR-like (nRF52840 ECB HW)
 *   Key = "REVITA_LORA_KEY1" (16 bytes)
 *   Keystream = AES-ECB(Key, Nonce)
 *   Cipher = Plain XOR Keystream
 *
 * Error Protection (3 layers):
 *   1. LoRa FEC: Coding Rate 4/6 (50% redundancy)
 *   2. LoRa HW CRC: 16-bit at PHY layer
 *   3. App CRC-16: integrity after decryption
 */

#define LORA_PAYLOAD_SIZE    16
#define LORA_FRAME_MAGIC     0xAA
#define LORA_FRAME_TYPE_CNT  0x01
#define LORA_FLAG_ENCRYPTED  0x01

/* LoRa parameters */
#define LORA_FREQ_DEFAULT    922100000  /* 922.1 MHz */
#define LORA_BW_DEFAULT      0         /* 125 kHz */
#define LORA_SF_DEFAULT      9
#define LORA_CR_DEFAULT      2         /* 4/6 */
#define LORA_POWER_DEFAULT   14        /* dBm */
#define LORA_PREAMBLE_LEN    8

/* LoRa state */
struct lora_status {
	uint16_t state;       /* 0=IDLE, 1=RX_LISTENING */
	uint16_t freq;        /* MHz code (e.g. 922) */
	int16_t  power;       /* dBm */
	int16_t  rssi;        /* last RSSI dBm */
	int16_t  snr;         /* last SNR x10 */
	uint16_t tx_cnt;
	uint16_t rx_cnt;
	uint16_t rx_err_cnt;  /* CRC fail / decrypt fail */
	uint32_t last_count;  /* received count value */
	uint8_t  last_seq;
	uint32_t last_uptime; /* received uptime seconds */
};

int lora_module_init(void);
int lora_rx_start(void);
int lora_rx_stop(void);
const struct lora_status *lora_get_status(void);

#endif
