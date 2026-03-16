#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

/* Custom Service UUID (NUS-compatible) */
#define BT_UUID_CUSTOM_SVC_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_CUSTOM_SVC BT_UUID_DECLARE_128(BT_UUID_CUSTOM_SVC_VAL)

/* TX char (peripheral -> central): notify */
#define BT_UUID_TX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_TX_CHAR BT_UUID_DECLARE_128(BT_UUID_TX_CHAR_VAL)

/* RX char (central -> peripheral): write */
#define BT_UUID_RX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_RX_CHAR BT_UUID_DECLARE_128(BT_UUID_RX_CHAR_VAL)

#define BUF_SIZE 256

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
static struct bt_conn *current_conn;
static bool notify_enabled;

/* LED0: advertising, LED1: connected, LED2: UART TX, LED3: BLE RX */
static const struct gpio_dt_spec led_adv = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_conn = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_uart = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static const struct gpio_dt_spec led_ble = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);

/* Double buffer: ISR writes to uart_buf, main thread reads from tx_buf */
static uint8_t uart_buf[BUF_SIZE];
static volatile uint8_t uart_pos;
static uint8_t tx_buf[BUF_SIZE];
static volatile uint8_t tx_len;

static K_SEM_DEFINE(tx_sem, 0, 1);

static void tx_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

/* RX write callback: central -> peripheral, output to UART */
static ssize_t on_rx_write(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   const void *buf, uint16_t len,
			   uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	for (uint16_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, data[i]);
	}

	gpio_pin_toggle_dt(&led_ble);

	return len;
}

BT_GATT_SERVICE_DEFINE(custom_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CUSTOM_SVC),
	BT_GATT_CHARACTERISTIC(BT_UUID_TX_CHAR,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       NULL, NULL, NULL),
	BT_GATT_CCC(tx_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_RX_CHAR,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, on_rx_write, NULL),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SVC_VAL),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t att_err,
			    struct bt_gatt_exchange_params *params)
{
}

static struct bt_gatt_exchange_params mtu_params = {
	.func = mtu_exchange_cb,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		return;
	}
	current_conn = bt_conn_ref(conn);
	gpio_pin_set_dt(&led_adv, 0);
	gpio_pin_set_dt(&led_conn, 1);

	/* Request larger MTU */
	bt_gatt_exchange_mtu(conn, &mtu_params);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}
	gpio_pin_set_dt(&led_conn, 0);
	gpio_pin_set_dt(&led_adv, 1);

	bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			sd, ARRAY_SIZE(sd));
}

BT_CONN_CB_DEFINE(conn_cb) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void uart_isr(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		uint8_t byte;

		if (uart_fifo_read(dev, &byte, 1) != 1) {
			continue;
		}

		if (uart_pos < BUF_SIZE) {
			uart_buf[uart_pos++] = byte;
		}

		/* Send on newline or buffer full */
		if (byte == '\r' || byte == '\n' || uart_pos >= BUF_SIZE) {
			memcpy(tx_buf, uart_buf, uart_pos);
			tx_len = uart_pos;
			uart_pos = 0;
			k_sem_give(&tx_sem);
		}
	}
}

int main(void)
{
	gpio_pin_configure_dt(&led_adv, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_conn, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_uart, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_ble, GPIO_OUTPUT_INACTIVE);

	if (!device_is_ready(uart_dev)) {
		return -1;
	}

	if (bt_enable(NULL)) {
		return -1;
	}

	if (bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			    sd, ARRAY_SIZE(sd))) {
		return -1;
	}

	gpio_pin_set_dt(&led_adv, 1);

	uart_irq_callback_set(uart_dev, uart_isr);
	uart_irq_rx_enable(uart_dev);

	while (1) {
		k_sem_take(&tx_sem, K_FOREVER);

		if (tx_len > 0 && current_conn && notify_enabled) {
			uint16_t mtu = bt_gatt_get_mtu(current_conn);
			uint8_t max_chunk = (mtu > 3) ? (mtu - 3) : 20;
			uint8_t off = 0;

			while (off < tx_len) {
				uint8_t chunk = tx_len - off;
				int ret;

				if (chunk > max_chunk) {
					chunk = max_chunk;
				}

				/* Retry up to 10 times on failure */
				for (int retry = 0; retry < 10; retry++) {
					ret = bt_gatt_notify(current_conn,
							     &custom_svc.attrs[1],
							     &tx_buf[off],
							     chunk);
					if (ret == 0) {
						break;
					}
					k_msleep(10);
				}

				off += chunk;
			}
			gpio_pin_toggle_dt(&led_uart);
		}
	}

	return 0;
}
