/*
 * System Test - Link (RX -> loopback TX)
 *
 * 동작:
 *   1. LoRa RX: Tower에서 보낸 메시지 수신
 *   2. Debug 출력: 수신 데이터를 printk (uart1)로 출력
 *   3. LoRa TX: "LINK:" prefix 붙여서 loopback 송신
 *   4. 다시 RX 모드로 전환
 *
 * LoRa: 922.1 MHz, SF9, BW125, CR4/6
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <string.h>
#include <stdio.h>

/* ── LoRa parameters ── */
#define LORA_FREQ       922100000
#define LORA_SF         12
#define LORA_BW         BW_125_KHZ
#define LORA_CR         CR_4_6
#define LORA_POWER      14
#define LORA_PREAMBLE   8
#define LORA_PAYLOAD    32

/* ── LoRa device ── */
static const struct device *lora_dev;

/* ── LoRa config ── */
static int lora_configure(bool tx)
{
	struct lora_modem_config config = {
		.frequency = LORA_FREQ,
		.bandwidth = LORA_BW,
		.datarate = LORA_SF,
		.coding_rate = LORA_CR,
		.preamble_len = LORA_PREAMBLE,
		.tx_power = LORA_POWER,
		.tx = tx,
		.iq_inverted = false,
		.public_network = false,
	};
	return lora_config(lora_dev, &config);
}

int main(void)
{
	uint8_t buf[LORA_PAYLOAD];
	int16_t rssi;
	int8_t snr;
	int ret;
	uint32_t rx_count = 0;
	uint32_t wait_count = 0;

	printk("\n========================================\n");
	printk("  System Test - Link v1.0\n");
	printk("  LoRa RX -> Debug + Loopback TX\n");
	printk("========================================\n");

	/* LoRa init */
	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("[LoRa] Device not ready!\n");
		return -1;
	}
	printk("[LoRa] SX1262 ready\n");

	/* SX1262 안정화 */
	k_sleep(K_SECONDS(1));

	/* Radio warmup: TX 한 번 수행 */
	lora_configure(true);
	uint8_t test[] = "LINK:BOOT";
	ret = lora_send(lora_dev, test, 9);
	printk("[Link] Warmup TX: %d\n", ret);
	k_sleep(K_MSEC(500));

	/* RX config (1회) */
	ret = lora_configure(false);
	printk("[Link] RX config: %d\n", ret);

	printk("[Link-RX] Listening...\n");

	while (1) {
		ret = lora_recv(lora_dev, buf, LORA_PAYLOAD,
				K_MSEC(5000), &rssi, &snr);

		if (ret == -EAGAIN) {
			wait_count++;
			printk("[Link-RX] waiting... (%u)\n", wait_count);
			/* RX config 없이 그냥 다시 recv */
			continue;
		}
		if (ret < 0) {
			printk("[Link-RX] Error: %d, re-config RX\n", ret);
			k_sleep(K_MSEC(200));
			lora_configure(false);
			continue;
		}

		/* 수신 성공 */
		rx_count++;
		buf[ret < LORA_PAYLOAD ? ret : LORA_PAYLOAD - 1] = '\0';

		printk("[Link-RX] #%u len=%d RSSI=%d SNR=%d data=\"%s\"\n",
		       rx_count, ret, rssi, snr, buf);

		/* Loopback TX */
		k_sleep(K_MSEC(100));

		if (lora_configure(true) < 0) {
			printk("[Link-TX] Config failed\n");
			lora_configure(false);
			continue;
		}

		uint8_t tx_buf[LORA_PAYLOAD];
		memset(tx_buf, 0, LORA_PAYLOAD);
		int tx_len = snprintf((char *)tx_buf, LORA_PAYLOAD,
				      "LINK:%s", buf);

		printk("[Link-TX] Loopback \"%s\"...\n", tx_buf);

		ret = lora_send(lora_dev, tx_buf, tx_len);
		if (ret < 0) {
			printk("[Link-TX] Failed: %d\n", ret);
		} else {
			printk("[Link-TX] OK\n");
		}

		/* RX 모드 복귀 */
		lora_configure(false);
		printk("[Link-RX] Listening...\n");
	}

	return 0;
}
