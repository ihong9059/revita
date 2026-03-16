/*
 * BLE Peripheral (RX) - Target2
 * - Advertises as "REVITA_RX"
 * - Receives counter value from Central
 * - Echoes back the received value via Notify
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

/* Custom Service UUID: 12345678-1234-5678-1234-56789abcdef0 */
#define BT_UUID_REVITA_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_REVITA_SERVICE BT_UUID_DECLARE_128(BT_UUID_REVITA_SERVICE_VAL)

/* RX Characteristic UUID: 12345678-1234-5678-1234-56789abcdef1 (Write) */
#define BT_UUID_REVITA_RX_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_REVITA_RX BT_UUID_DECLARE_128(BT_UUID_REVITA_RX_VAL)

/* TX Characteristic UUID: 12345678-1234-5678-1234-56789abcdef2 (Notify) */
#define BT_UUID_REVITA_TX_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)
#define BT_UUID_REVITA_TX BT_UUID_DECLARE_128(BT_UUID_REVITA_TX_VAL)

static struct bt_conn *current_conn;
static bool notify_enabled;
static const int8_t tx_power_level = 8; /* +8 dBm */

/* Forward declaration for TX attribute */
static ssize_t rx_write_cb(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len,
                           uint16_t offset, uint8_t flags);

/* CCC changed callback */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("Notify %s\n", notify_enabled ? "ON" : "OFF");
}

/* GATT Service Definition */
BT_GATT_SERVICE_DEFINE(revita_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_REVITA_SERVICE),
    /* RX Characteristic - for receiving data (index 1, 2) */
    BT_GATT_CHARACTERISTIC(BT_UUID_REVITA_RX,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           NULL, rx_write_cb, NULL),
    /* TX Characteristic - for sending notifications (index 3, 4) */
    BT_GATT_CHARACTERISTIC(BT_UUID_REVITA_TX,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* RX Characteristic write callback - echo back via notify on TX characteristic */
static ssize_t rx_write_cb(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           const void *buf, uint16_t len,
                           uint16_t offset, uint8_t flags)
{
    if (len >= sizeof(uint32_t) && notify_enabled && current_conn) {
        /* Get TX characteristic attribute (index 4 in service = attrs[4]) */
        const struct bt_gatt_attr *tx_attr = &revita_svc.attrs[4];
        bt_gatt_notify(conn, tx_attr, buf, len);
    }
    return len;
}

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "REVITA_RX", 9),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REVITA_SERVICE_VAL),
};

/* Advertising parameters */
static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL
);

static int start_advertising(void)
{
    return bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
        return;
    }

    current_conn = bt_conn_ref(conn);
    printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    notify_enabled = false;

    /* Restart advertising */
    int err = start_advertising();
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int main(void)
{
    int err;

    printk("\n=================================\n");
    printk("BLE Peripheral (RX) - Target2\n");
    printk("TX Power: +%d dBm\n", tx_power_level);
    printk("=================================\n\n");

    /* Initialize Bluetooth */
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }

    printk("Bluetooth initialized\n");

    /* Start advertising */
    err = start_advertising();
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return 0;
    }

    printk("Advertising started as \"REVITA_RX\"\n");
    printk("Waiting for connection...\n");

    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
