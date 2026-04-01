/*
 * LoRa End Device - RX module
 * Receives encrypted 16-byte frames from Gateway
 */

#include "lora.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <hal/nrf_ecb.h>
#include <string.h>

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

/* ECB data block (must be aligned) */
static struct {
	uint8_t key[16];
	uint8_t cleartext[16];
	uint8_t ciphertext[16];
} __attribute__((aligned(4))) ecb_data;

static uint8_t keystream[16];

/* LoRa device */
static const struct device *lora_dev;
static struct lora_status status;
static volatile bool rx_running;

/* RX thread */
#define RX_STACK_SIZE 2048
#define RX_PRIORITY   7
static K_THREAD_STACK_DEFINE(rx_stack, RX_STACK_SIZE);
static struct k_thread rx_thread_data;
static k_tid_t rx_tid;

/* CRC-16 (same polynomial as Modbus: 0xA001) */
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

/* XOR decrypt (same as encrypt - symmetric) */
static void crypto_xor(uint8_t *data, uint8_t len)
{
	for (int i = 0; i < len && i < 16; i++) {
		data[i] ^= keystream[i];
	}
}

/* Parse and validate decrypted frame */
static bool parse_frame(const uint8_t *frame)
{
	if (frame[0] != LORA_FRAME_MAGIC) {
		return false;
	}
	if (frame[1] != LORA_FRAME_TYPE_CNT) {
		return false;
	}

	/* Verify app CRC-16 (over bytes 0-11) */
	uint16_t rx_crc = ((uint16_t)frame[12] << 8) | frame[13];
	uint16_t calc_crc = crc16(frame, 12);
	if (rx_crc != calc_crc) {
		return false;
	}

	/* Extract fields */
	status.last_seq = frame[2];
	status.last_count = ((uint32_t)frame[4] << 24) |
			    ((uint32_t)frame[5] << 16) |
			    ((uint32_t)frame[6] << 8) |
			    frame[7];
	status.last_uptime = ((uint32_t)frame[8] << 24) |
			     ((uint32_t)frame[9] << 16) |
			     ((uint32_t)frame[10] << 8) |
			     frame[11];

	return true;
}

/* RX thread function */
static void rx_thread_func(void *p1, void *p2, void *p3)
{
	uint8_t buf[LORA_PAYLOAD_SIZE];
	int16_t rssi;
	int8_t snr;
	int ret;

	printk("[LoRa] RX thread started\n");

	while (rx_running) {
		ret = lora_recv(lora_dev, buf, LORA_PAYLOAD_SIZE,
				K_MSEC(3000), &rssi, &snr);
		if (!rx_running) {
			break;
		}

		if (ret < 0) {
			/* Timeout or error - just retry */
			continue;
		}

		if (ret != LORA_PAYLOAD_SIZE) {
			printk("[LoRa] RX bad len=%d\n", ret);
			status.rx_err_cnt++;
			continue;
		}

		/* Update RF stats */
		status.rssi = rssi;
		status.snr = snr * 10;  /* ×10 for register */

		/* Decrypt */
		crypto_xor(buf, LORA_PAYLOAD_SIZE);

		/* Parse and validate */
		if (parse_frame(buf)) {
			status.rx_cnt++;
			printk("[LoRa] RX #%u seq=%u count=%u uptime=%us RSSI=%d SNR=%d\n",
			       status.rx_cnt, status.last_seq,
			       status.last_count, status.last_uptime,
			       rssi, snr);
		} else {
			status.rx_err_cnt++;
			printk("[LoRa] RX CRC/frame error (RSSI=%d)\n", rssi);
		}
	}

	printk("[LoRa] RX thread stopped\n");
}

int lora_module_init(void)
{
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("[LoRa] Device not ready!\n");
		return -1;
	}

	/* Generate AES keystream */
	generate_keystream();

	memset(&status, 0, sizeof(status));
	status.freq = 922;  /* 922.1 MHz */
	status.power = LORA_POWER_DEFAULT;

	printk("[LoRa] Module init OK (SX1262)\n");
	return 0;
}

int lora_rx_start(void)
{
	if (rx_running) {
		return 0;
	}

	struct lora_modem_config config = {
		.frequency = LORA_FREQ_DEFAULT,
		.bandwidth = BW_125_KHZ,
		.datarate = LORA_SF_DEFAULT,
		.coding_rate = CR_4_6,
		.preamble_len = LORA_PREAMBLE_LEN,
		.tx_power = LORA_POWER_DEFAULT,
		.tx = false,
		.iq_inverted = false,
		.public_network = false,
	};

	int ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		printk("[LoRa] Config failed: %d\n", ret);
		return ret;
	}

	rx_running = true;
	status.state = 1;  /* RX_LISTENING */

	rx_tid = k_thread_create(&rx_thread_data, rx_stack,
				 K_THREAD_STACK_SIZEOF(rx_stack),
				 rx_thread_func, NULL, NULL, NULL,
				 RX_PRIORITY, 0, K_NO_WAIT);

	printk("[LoRa] RX started (922.1 MHz, SF9, BW125, CR4/6)\n");
	return 0;
}

int lora_rx_stop(void)
{
	if (!rx_running) {
		return 0;
	}

	rx_running = false;
	k_thread_join(&rx_thread_data, K_SECONDS(5));
	status.state = 0;  /* IDLE */

	printk("[LoRa] RX stopped\n");
	return 0;
}

const struct lora_status *lora_get_status(void)
{
	return &status;
}
