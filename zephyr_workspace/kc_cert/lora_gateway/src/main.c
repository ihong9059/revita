/*
 * LoRa Gateway - TX count broadcast every 2 seconds
 *
 * Frame Structure (16 bytes):
 *   [0]     Magic: 0xAA
 *   [1]     Type:  0x01 (count broadcast)
 *   [2]     Seq:   0-255 (wrapping)
 *   [3]     Flags: 0x01 (encrypted)
 *   [4-7]   Count (uint32 big-endian)
 *   [8-11]  Uptime seconds (uint32 big-endian)
 *   [12-13] CRC-16 (bytes 0-11)
 *   [14-15] Padding 0x00
 *
 * Encryption: AES-128 keystream XOR (nRF ECB HW)
 * Error Protection: App CRC-16 + LoRa FEC (CR4/6) + LoRa HW CRC
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <hal/nrf_ecb.h>
#include <string.h>

#define PAYLOAD_SIZE     16
#define TX_INTERVAL_MS   2000
#define FRAME_MAGIC      0xAA
#define FRAME_TYPE_CNT   0x01
#define FLAG_ENCRYPTED   0x01

/* LoRa parameters - must match End Device */
#define LORA_FREQ        922100000  /* 922.1 MHz */
#define LORA_TX_POWER    14         /* dBm */

/* AES-128 Key: "REVITA_LORA_KEY1" */
static const uint8_t aes_key[16] = {
	0x52, 0x45, 0x56, 0x49, 0x54, 0x41, 0x5F, 0x4C,
	0x4F, 0x52, 0x41, 0x5F, 0x4B, 0x45, 0x59, 0x31
};

/* AES Nonce: "REVITA_NONCE_001" */
static const uint8_t aes_nonce[16] = {
	0x52, 0x45, 0x56, 0x49, 0x54, 0x41, 0x5F, 0x4E,
	0x4F, 0x4E, 0x43, 0x45, 0x5F, 0x30, 0x30, 0x31
};

/* ECB data block */
static struct {
	uint8_t key[16];
	uint8_t cleartext[16];
	uint8_t ciphertext[16];
} __attribute__((aligned(4))) ecb_data;

static uint8_t keystream[16];

/* CRC-16 (Modbus polynomial 0xA001) */
static uint16_t crc16(const uint8_t *buf, uint16_t len)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < len; i++) {
		crc ^= buf[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

/* Generate AES keystream using nRF ECB hardware */
static void generate_keystream(void)
{
	memcpy(ecb_data.key, aes_key, 16);
	memcpy(ecb_data.cleartext, aes_nonce, 16);

	NRF_ECB->ECBDATAPTR = (uint32_t)&ecb_data;
	NRF_ECB->EVENTS_ENDECB = 0;
	NRF_ECB->EVENTS_ERRORECB = 0;
	NRF_ECB->TASKS_STARTECB = 1;

	while (!NRF_ECB->EVENTS_ENDECB && !NRF_ECB->EVENTS_ERRORECB) {
		/* wait */
	}

	memcpy(keystream, ecb_data.ciphertext, 16);
}

/* XOR encrypt */
static void crypto_xor(uint8_t *data, uint8_t len)
{
	for (int i = 0; i < len && i < 16; i++) {
		data[i] ^= keystream[i];
	}
}

/* Build 16-byte frame */
static void build_frame(uint8_t *buf, uint8_t seq, uint32_t count, uint32_t uptime)
{
	buf[0] = FRAME_MAGIC;
	buf[1] = FRAME_TYPE_CNT;
	buf[2] = seq;
	buf[3] = FLAG_ENCRYPTED;

	/* Count (big-endian) */
	buf[4] = (count >> 24) & 0xFF;
	buf[5] = (count >> 16) & 0xFF;
	buf[6] = (count >> 8) & 0xFF;
	buf[7] = count & 0xFF;

	/* Uptime seconds (big-endian) */
	buf[8] = (uptime >> 24) & 0xFF;
	buf[9] = (uptime >> 16) & 0xFF;
	buf[10] = (uptime >> 8) & 0xFF;
	buf[11] = uptime & 0xFF;

	/* CRC-16 over bytes 0-11 (big-endian) */
	uint16_t crc = crc16(buf, 12);
	buf[12] = (crc >> 8) & 0xFF;
	buf[13] = crc & 0xFF;

	/* Padding */
	buf[14] = 0x00;
	buf[15] = 0x00;
}

int main(void)
{
	const struct device *lora_dev;
	int ret;

	printk("\n========================================\n");
	printk("  LoRa Gateway - Count Broadcaster\n");
	printk("  REVITA / RAK4631 (SX1262)\n");
	printk("========================================\n");
	printk("  Freq:  922.1 MHz\n");
	printk("  Power: %d dBm\n", LORA_TX_POWER);
	printk("  SF9 / BW125 / CR4/6\n");
	printk("  Payload: %d bytes (AES-128 encrypted)\n", PAYLOAD_SIZE);
	printk("  Interval: %d ms\n", TX_INTERVAL_MS);
	printk("========================================\n\n");

	/* Init LoRa */
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("[GW] LoRa device not ready!\n");
		return -1;
	}
	printk("[GW] LoRa device ready\n");

	struct lora_modem_config config = {
		.frequency = LORA_FREQ,
		.bandwidth = BW_125_KHZ,
		.datarate = SF_9,
		.coding_rate = CR_4_6,
		.preamble_len = 8,
		.tx_power = LORA_TX_POWER,
		.tx = true,
		.iq_inverted = false,
		.public_network = false,
	};

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		printk("[GW] LoRa config failed: %d\n", ret);
		return -1;
	}
	printk("[GW] LoRa configured OK\n");

	/* Generate AES keystream */
	generate_keystream();
	printk("[GW] AES-128 keystream generated\n");

	/* TX loop */
	uint32_t count = 0;
	uint8_t seq = 0;
	uint8_t payload[PAYLOAD_SIZE];

	printk("[GW] Starting TX loop...\n\n");

	while (1) {
		uint32_t uptime = (uint32_t)(k_uptime_get() / 1000);

		/* Build frame */
		build_frame(payload, seq, count, uptime);

		/* Encrypt */
		crypto_xor(payload, PAYLOAD_SIZE);

		/* Send */
		ret = lora_send(lora_dev, payload, PAYLOAD_SIZE);
		if (ret < 0) {
			printk("[GW] TX failed: %d, reconfiguring...\n", ret);
			/* Radio stuck - reconfigure to reset state */
			lora_config(lora_dev, &config);
			k_sleep(K_MSEC(100));
			continue;
		}

		printk("[GW] TX #%u seq=%u count=%u uptime=%us\n",
		       count + 1, seq, count, uptime);

		count++;
		seq++;  /* wraps at 255→0 */

		/* Reconfigure before next TX to prevent radio stuck */
		lora_config(lora_dev, &config);

		k_sleep(K_MSEC(TX_INTERVAL_MS));
	}

	return 0;
}
