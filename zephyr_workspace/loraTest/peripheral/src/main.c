/*
 * LoRa + BLE Peripheral (RX) - Target2
 * - BLE: Advertises as "REVITA_RX", receives and echoes via notify
 * - LoRa: Receives and echoes back
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

/* LoRa device */
#define LORA_DEV DEVICE_DT_GET(DT_ALIAS(lora0))

/* LoRa configuration */
#define LORA_FREQUENCY      923000000
#define LORA_BANDWIDTH      BW_125_KHZ
#define LORA_SPREADING      SF_7
#define LORA_CODING_RATE    CR_4_5
#define LORA_TX_POWER       14

/* BLE UUIDs */
#define BT_UUID_REVITA_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)
#define BT_UUID_REVITA_SERVICE BT_UUID_DECLARE_128(BT_UUID_REVITA_SERVICE_VAL)

#define BT_UUID_REVITA_RX_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)
#define BT_UUID_REVITA_RX BT_UUID_DECLARE_128(BT_UUID_REVITA_RX_VAL)

#define BT_UUID_REVITA_TX_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)
#define BT_UUID_REVITA_TX BT_UUID_DECLARE_128(BT_UUID_REVITA_TX_VAL)

/* Global state */
static const struct device *lora_dev;
static bool lora_ready;
static struct bt_conn *ble_conn;
static bool notify_enabled;

/* LoRa packet structure */
struct lora_packet {
    uint32_t counter;
    uint8_t type;  /* 'T' for TX, 'R' for RX echo */
};

/* BLE CCC callback */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    printk("BLE notify %s\n", notify_enabled ? "ON" : "OFF");
}

/* Forward declaration */
static ssize_t ble_rx_write_cb(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len,
                               uint16_t offset, uint8_t flags);

/* BLE GATT Service */
BT_GATT_SERVICE_DEFINE(revita_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_REVITA_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_REVITA_RX,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           NULL, ble_rx_write_cb, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_REVITA_TX,
                           BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE,
                           NULL, NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* BLE RX write callback - echo back */
static ssize_t ble_rx_write_cb(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len,
                               uint16_t offset, uint8_t flags)
{
    if (len >= sizeof(uint32_t)) {
        uint32_t received;
        memcpy(&received, buf, sizeof(received));
        printk("BLE RX: %u\n", received);

        if (notify_enabled && ble_conn) {
            const struct bt_gatt_attr *tx_attr = &revita_svc.attrs[4];
            bt_gatt_notify(conn, tx_attr, buf, len);
            printk("BLE TX (echo): %u\n", received);
        }
    }
    return len;
}

/* BLE Advertising */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "REVITA_RX", 9),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_REVITA_SERVICE_VAL),
};

static struct bt_le_adv_param adv_param = BT_LE_ADV_PARAM_INIT(
    BT_LE_ADV_OPT_CONN,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL
);

static void start_advertising(void)
{
    bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    printk("BLE advertising\n");
}

static void ble_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) return;
    ble_conn = bt_conn_ref(conn);
    printk("BLE connected\n");
}

static void ble_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("BLE disconnected\n");
    if (ble_conn) {
        bt_conn_unref(ble_conn);
        ble_conn = NULL;
    }
    notify_enabled = false;
    start_advertising();
}

BT_CONN_CB_DEFINE(conn_cbs) = {
    .connected = ble_connected,
    .disconnected = ble_disconnected,
};

/* LoRa init */
static int lora_init(void)
{
    printk("LoRa: Checking device...\n");

    if (!device_is_ready(lora_dev)) {
        printk("LoRa: Device not ready\n");
        return -1;
    }
    printk("LoRa: Device ready\n");

    struct lora_modem_config config = {
        .frequency = LORA_FREQUENCY,
        .bandwidth = LORA_BANDWIDTH,
        .datarate = LORA_SPREADING,
        .coding_rate = LORA_CODING_RATE,
        .preamble_len = 8,
        .tx_power = LORA_TX_POWER,
        .tx = false,  /* Start in RX mode */
    };

    printk("LoRa: Configuring...\n");
    int ret = lora_config(lora_dev, &config);
    if (ret < 0) {
        printk("LoRa: Config failed: %d\n", ret);
        return ret;
    }

    printk("LoRa: Initialized %d MHz, SF%d, BW125kHz\n",
           LORA_FREQUENCY / 1000000, 7);
    return 0;
}

/* LoRa send echo */
static void lora_send_echo(uint32_t cnt)
{
    struct lora_modem_config config = {
        .frequency = LORA_FREQUENCY,
        .bandwidth = LORA_BANDWIDTH,
        .datarate = LORA_SPREADING,
        .coding_rate = LORA_CODING_RATE,
        .preamble_len = 8,
        .tx_power = LORA_TX_POWER,
        .tx = true,
    };
    lora_config(lora_dev, &config);

    struct lora_packet pkt = {
        .counter = cnt,
        .type = 'R'  /* Echo response */
    };

    int ret = lora_send(lora_dev, (uint8_t *)&pkt, sizeof(pkt));
    if (ret == 0) {
        printk("LoRa TX (echo): %u\n", cnt);
    }
}

/* LoRa RX thread */
static void lora_rx_thread(void)
{
    uint8_t buf[32];
    int16_t rssi;
    int8_t snr;

    /* Wait for LoRa init */
    while (!lora_ready) {
        k_sleep(K_MSEC(100));
    }

    printk("LoRa: RX thread started\n");

    while (1) {
        /* Configure for RX */
        struct lora_modem_config config = {
            .frequency = LORA_FREQUENCY,
            .bandwidth = LORA_BANDWIDTH,
            .datarate = LORA_SPREADING,
            .coding_rate = LORA_CODING_RATE,
            .preamble_len = 8,
            .tx_power = LORA_TX_POWER,
            .tx = false,
        };
        lora_config(lora_dev, &config);

        int len = lora_recv(lora_dev, buf, sizeof(buf), K_MSEC(500), &rssi, &snr);
        if (len > 0 && len >= sizeof(struct lora_packet)) {
            struct lora_packet *pkt = (struct lora_packet *)buf;
            if (pkt->type == 'T') {
                printk("LoRa RX: %u | RSSI: %d | SNR: %d -> Echo\n",
                       pkt->counter, rssi, snr);
                /* Send echo back */
                k_sleep(K_MSEC(10));
                lora_send_echo(pkt->counter);
            }
        }
    }
}

K_THREAD_DEFINE(lora_rx_tid, 2048, lora_rx_thread, NULL, NULL, NULL, 7, 0, 0);

int main(void)
{
    printk("\n========================================\n");
    printk("LoRa + BLE Peripheral (RX) - Target2\n");
    printk("========================================\n\n");

    /* Init BLE FIRST */
    printk("Initializing BLE...\n");
    if (bt_enable(NULL)) {
        printk("BLE init failed!\n");
        return 0;
    }
    printk("BLE initialized\n");
    start_advertising();

    /* Init LoRa after BLE is running */
    printk("Initializing LoRa...\n");
    lora_dev = LORA_DEV;
    if (lora_init() < 0) {
        printk("LoRa init failed! Continuing with BLE only.\n");
    } else {
        lora_ready = true;
        printk("LoRa ready\n");
    }

    /* Main loop */
    while (1) {
        k_sleep(K_SECONDS(10));
    }

    return 0;
}
