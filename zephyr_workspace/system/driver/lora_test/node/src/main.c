/*
 * LoRa basic test - Node (Link)
 *
 * 동작:
 *   1. RX: Gateway에서 보낸 메시지 수신 (5초 timeout)
 *   2. RSSI/SNR/원본 데이터 출력
 *   3. TX: "ND:" prefix 붙여서 echo 송신
 *   4. RX 모드 복귀, 반복
 *
 * 주의: SX1262 부팅 직후 RX 진입 시 -EIO 발생 가능 → warmup TX 1회 수행
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <stdio.h>

/* ── LoRa parameters (Gateway와 동일해야 통신 가능) ── */
#define LORA_FREQ       922100000
#define LORA_SF         12
#define LORA_BW         BW_125_KHZ
#define LORA_CR         CR_4_6
#define LORA_POWER      14
#define LORA_PREAMBLE   8
#define LORA_PAYLOAD    32

#define RX_TIMEOUT_MS   5000

static const struct device *lora_dev;
static const struct device *console_dev;

/* Console RX 스레드: cutecom에서 받은 문자를 echo 출력하여 RX 핀 동작 검증 */
static void console_rx_thread(void *p1, void *p2, void *p3)
{
	unsigned char c;
	uint32_t total = 0;

	while (1) {
		if (console_dev && uart_poll_in(console_dev, &c) == 0) {
			total++;
			printk("[ND-UART-RX] got 0x%02X '%c' (total=%u)\n",
			       c,
			       (c >= 0x20 && c < 0x7F) ? c : '.',
			       total);
		} else {
			k_sleep(K_MSEC(20));
		}
	}
}

K_THREAD_DEFINE(console_rx_tid, 1024, console_rx_thread,
		NULL, NULL, NULL, 7, 0, 0);

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
	uint8_t rx_buf[LORA_PAYLOAD];
	uint8_t tx_buf[LORA_PAYLOAD];
	int16_t rssi;
	int8_t snr;
	int ret;
	uint32_t rx_count = 0, tx_count = 0;
	uint32_t rx_timeout = 0, rx_err = 0;

	printk("\n========================================\n");
	printk("  LoRa basic test - Node (Link)\n");
	printk("  freq=%u Hz SF%d BW125 CR4/6 pwr=%d dBm\n",
	       LORA_FREQ, LORA_SF, LORA_POWER);
	printk("========================================\n");

	console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	if (device_is_ready(console_dev)) {
		printk("[ND] console UART ready (type in cutecom to test RX)\n");
	} else {
		printk("[ND] console UART NOT ready\n");
	}

	lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("[ND] LoRa device not ready!\n");
		return -1;
	}
	printk("[ND] SX1262 ready\n");

	k_sleep(K_SECONDS(1));

	/* Warmup TX (radio 안정화) */
	lora_set_mode(true);
	uint8_t warm[] = "ND:BOOT";
	ret = lora_send(lora_dev, warm, sizeof(warm) - 1);
	printk("[ND] warmup TX: %d\n", ret);
	k_sleep(K_MSEC(500));

	/* RX 모드 진입 */
	if (lora_set_mode(false) < 0) {
		printk("[ND] initial RX config fail\n");
		return -1;
	}
	printk("[ND-RX] listening...\n");

	while (1) {
		ret = lora_recv(lora_dev, rx_buf, LORA_PAYLOAD,
				K_MSEC(RX_TIMEOUT_MS), &rssi, &snr);

		if (ret == -EAGAIN || ret == -ETIMEDOUT) {
			rx_timeout++;
			printk("[ND-RX] waiting... (timeout=%u)\n",
			       rx_timeout);
			continue;
		}
		if (ret < 0) {
			rx_err++;
			printk("[ND-RX] error %d (err_total=%u), re-config\n",
			       ret, rx_err);
			k_sleep(K_MSEC(200));
			lora_set_mode(false);
			continue;
		}

		rx_count++;
		rx_buf[ret < LORA_PAYLOAD ? ret : LORA_PAYLOAD - 1] = '\0';
		printk("\n[ND-RX] #%u len=%d RSSI=%d SNR=%d data=\"%s\"\n",
		       rx_count, ret, rssi, snr, rx_buf);

		/* === Echo TX === */
		k_sleep(K_MSEC(100));

		if (lora_set_mode(true) < 0) {
			printk("[ND-TX] config fail, back to RX\n");
			lora_set_mode(false);
			continue;
		}

		memset(tx_buf, 0, sizeof(tx_buf));
		int tx_len = snprintf((char *)tx_buf, LORA_PAYLOAD,
				      "ND:%s", rx_buf);

		printk("[ND-TX] echoing \"%s\" (%d bytes)\n", tx_buf, tx_len);

		ret = lora_send(lora_dev, tx_buf, tx_len);
		if (ret < 0) {
			printk("[ND-TX] send fail: %d\n", ret);
		} else {
			tx_count++;
			printk("[ND-TX] sent OK [tx_count=%u]\n", tx_count);
		}

		/* RX 모드 복귀 */
		if (lora_set_mode(false) < 0) {
			printk("[ND] RX restore fail\n");
		} else {
			printk("[ND-RX] listening...\n");
		}
	}

	return 0;
}
