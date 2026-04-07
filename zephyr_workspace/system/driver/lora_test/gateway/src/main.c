/*
 * LoRa basic test - Gateway (Tower)
 *
 * 동작 (단일 스레드, 순차 처리):
 *   1. TX: "GW:<count>" 메시지 송신
 *   2. RX: Node에서 echo된 메시지 5초 대기
 *   3. RSSI/SNR/통계 출력
 *   4. 1초 대기 후 반복
 *
 * REVITA protocol과 무관한 순수 LoRa wire-level test.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <string.h>
#include <stdio.h>

/* ── LoRa parameters ── */
#define LORA_FREQ       922100000
#define LORA_SF         12              /* SF7..SF12 — 안테나 약하면 SF12 */
#define LORA_BW         BW_125_KHZ
#define LORA_CR         CR_4_6
#define LORA_POWER      14              /* dBm */
#define LORA_PREAMBLE   8
#define LORA_PAYLOAD    32

#define RX_TIMEOUT_MS   5000
#define TX_INTERVAL_MS  1000

static const struct device *lora_dev;

static int lora_set_mode(bool tx)
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
	uint32_t count = 0;
	uint32_t tx_ok = 0, tx_fail = 0;
	uint32_t rx_ok = 0, rx_timeout = 0, rx_err = 0;

	printk("\n========================================\n");
	printk("  LoRa basic test - Gateway (Tower)\n");
	printk("  freq=%u Hz SF%d BW125 CR4/6 pwr=%d dBm\n",
	       LORA_FREQ, LORA_SF, LORA_POWER);
	printk("========================================\n");

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("[GW] LoRa device not ready!\n");
		return -1;
	}
	printk("[GW] SX1262 ready\n");

	k_sleep(K_SECONDS(1));

	while (1) {
		/* === TX === */
		if (lora_set_mode(true) < 0) {
			printk("[GW-TX] config fail\n");
			k_sleep(K_SECONDS(2));
			continue;
		}

		uint8_t tx_buf[LORA_PAYLOAD];
		memset(tx_buf, 0, sizeof(tx_buf));
		int tx_len = snprintf((char *)tx_buf, LORA_PAYLOAD,
				      "GW:%u", count);

		printk("\n[GW-TX] #%u sending \"%s\" (%d bytes)\n",
		       count, tx_buf, tx_len);

		int ret = lora_send(lora_dev, tx_buf, tx_len);
		if (ret < 0) {
			tx_fail++;
			printk("[GW-TX] send fail: %d  [tx_ok=%u tx_fail=%u]\n",
			       ret, tx_ok, tx_fail);
			k_sleep(K_SECONDS(2));
			count++;
			continue;
		}
		tx_ok++;
		printk("[GW-TX] sent OK  [tx_ok=%u tx_fail=%u]\n",
		       tx_ok, tx_fail);

		/* === RX (echo 대기) === */
		if (lora_set_mode(false) < 0) {
			printk("[GW-RX] config fail\n");
			k_sleep(K_SECONDS(2));
			count++;
			continue;
		}

		uint8_t rx_buf[LORA_PAYLOAD];
		int16_t rssi;
		int8_t snr;

		ret = lora_recv(lora_dev, rx_buf, LORA_PAYLOAD,
				K_MSEC(RX_TIMEOUT_MS), &rssi, &snr);

		if (ret == -EAGAIN || ret == -ETIMEDOUT) {
			rx_timeout++;
			printk("[GW-RX] timeout (no echo)  "
			       "[ok=%u to=%u err=%u]\n",
			       rx_ok, rx_timeout, rx_err);
		} else if (ret < 0) {
			rx_err++;
			printk("[GW-RX] error %d  [ok=%u to=%u err=%u]\n",
			       ret, rx_ok, rx_timeout, rx_err);
		} else {
			rx_ok++;
			rx_buf[ret < LORA_PAYLOAD ? ret : LORA_PAYLOAD - 1] = '\0';
			printk("[GW-RX] echo OK len=%d RSSI=%d SNR=%d "
			       "data=\"%s\"  [ok=%u to=%u err=%u]\n",
			       ret, rssi, snr, rx_buf,
			       rx_ok, rx_timeout, rx_err);
		}

		count++;
		k_sleep(K_MSEC(TX_INTERVAL_MS));
	}

	return 0;
}
