/*
 * LoRa + BLE Central (TX) - Target1
 * - BLE: Connects to "REVITA_RX", sends counter, receives echo
 * - LoRa: Sends counter, receives echo
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
static struct bt_uuid_128 revita_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));
static struct bt_uuid_128 revita_rx_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));
static struct bt_uuid_128 revita_tx_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

/* LoRa packet structure */
struct lora_packet {
    uint32_t counter;
    uint8_t type;  /* 'T' for TX, 'R' for RX echo */
};

/* Global state */
static const struct device *lora_dev;
static bool lora_ready;
static struct bt_conn *ble_conn;
static uint16_t ble_rx_handle;
static uint16_t ble_tx_handle;
static uint16_t ble_ccc_handle;
static bool ble_ready;
static uint32_t counter;

/* LoRa TX/RX now done sequentially in main loop */

/* BLE notification callback */
static uint8_t ble_notify_cb(struct bt_conn *conn,
                             struct bt_gatt_subscribe_params *params,
                             const void *data, uint16_t length)
{
    if (!data) {
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    if (length >= sizeof(uint32_t)) {
        uint32_t received;
        memcpy(&received, data, sizeof(received));
        printk("BLE RX: %u\n", received);
    }

    return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_subscribe_params ble_subscribe_params;

static void ble_subscribe(void)
{
    ble_subscribe_params.notify = ble_notify_cb;
    ble_subscribe_params.value_handle = ble_tx_handle;
    ble_subscribe_params.ccc_handle = ble_ccc_handle;
    ble_subscribe_params.value = BT_GATT_CCC_NOTIFY;

    int err = bt_gatt_subscribe(ble_conn, &ble_subscribe_params);
    if (err == 0) {
        printk("BLE subscribed\n");
        ble_ready = true;
    }
}

/* BLE send counter */
static void ble_send_counter(uint32_t cnt)
{
    if (!ble_conn || !ble_ready) return;

    int err = bt_gatt_write_without_response(ble_conn, ble_rx_handle,
                                              &cnt, sizeof(cnt), false);
    if (err == 0) {
        printk("BLE TX: %u\n", cnt);
    }
}

/* BLE discovery */
static uint16_t svc_start, svc_end;
static struct bt_gatt_discover_params disc_params;

static uint8_t char_disc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            struct bt_gatt_discover_params *params)
{
    if (!attr) {
        if (ble_rx_handle && ble_tx_handle) {
            ble_ccc_handle = ble_tx_handle + 1;
            printk("BLE handles: rx=%u, tx=%u, ccc=%u\n",
                   ble_rx_handle, ble_tx_handle, ble_ccc_handle);
            k_sleep(K_MSEC(100));
            ble_subscribe();
        }
        return BT_GATT_ITER_STOP;
    }

    struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
    if (bt_uuid_cmp(chrc->uuid, &revita_rx_uuid.uuid) == 0) {
        ble_rx_handle = chrc->value_handle;
    } else if (bt_uuid_cmp(chrc->uuid, &revita_tx_uuid.uuid) == 0) {
        ble_tx_handle = chrc->value_handle;
    }
    return BT_GATT_ITER_CONTINUE;
}

static uint8_t svc_disc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           struct bt_gatt_discover_params *params)
{
    if (!attr) {
        if (svc_start > 0) {
            disc_params.uuid = NULL;
            disc_params.func = char_disc_cb;
            disc_params.start_handle = svc_start;
            disc_params.end_handle = svc_end;
            disc_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
            bt_gatt_discover(conn, &disc_params);
        }
        return BT_GATT_ITER_STOP;
    }

    struct bt_gatt_service_val *svc = (struct bt_gatt_service_val *)attr->user_data;
    if (bt_uuid_cmp(svc->uuid, &revita_service_uuid.uuid) == 0) {
        svc_start = attr->handle;
        svc_end = svc->end_handle;
        printk("BLE service found [%u-%u]\n", svc_start, svc_end);
    }
    return BT_GATT_ITER_CONTINUE;
}

static void ble_discover(struct bt_conn *conn)
{
    svc_start = svc_end = 0;
    ble_rx_handle = ble_tx_handle = ble_ccc_handle = 0;

    disc_params.uuid = NULL;
    disc_params.func = svc_disc_cb;
    disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    disc_params.type = BT_GATT_DISCOVER_PRIMARY;
    bt_gatt_discover(conn, &disc_params);
}

/* BLE scan */
static void start_scan(void);

static void device_found(const bt_addr_le_t *addr, int8_t rssi,
                         uint8_t type, struct net_buf_simple *ad)
{
    if (ble_conn) return;

    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        if (len == 0 || len > ad->len) break;
        uint8_t ad_type = net_buf_simple_pull_u8(ad);
        len--;

        if ((ad_type == BT_DATA_NAME_COMPLETE) &&
            len == 9 && memcmp(ad->data, "REVITA_RX", 9) == 0) {

            bt_le_scan_stop();
            printk("BLE found REVITA_RX\n");

            struct bt_conn_le_create_param cp = BT_CONN_LE_CREATE_PARAM_INIT(
                BT_CONN_LE_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
            struct bt_le_conn_param conn_p = BT_LE_CONN_PARAM_INIT(80, 80, 0, 400);

            if (bt_conn_le_create(addr, &cp, &conn_p, &ble_conn)) {
                start_scan();
            }
            return;
        }
        net_buf_simple_pull(ad, len);
    }
}

static void start_scan(void)
{
    struct bt_le_scan_param sp = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };
    bt_le_scan_start(&sp, device_found);
    printk("BLE scanning...\n");
}

static void ble_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        ble_conn = NULL;
        start_scan();
        return;
    }
    printk("BLE connected\n");
    k_sleep(K_MSEC(500));
    ble_discover(conn);
}

static void ble_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("BLE disconnected\n");
    if (ble_conn) {
        bt_conn_unref(ble_conn);
        ble_conn = NULL;
    }
    ble_ready = false;
    start_scan();
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
        .tx = false,
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

/* LoRa send counter and receive echo */
static void lora_send_counter(uint32_t cnt)
{
    if (!lora_ready) return;

    /* TX */
    struct lora_modem_config tx_config = {
        .frequency = LORA_FREQUENCY,
        .bandwidth = LORA_BANDWIDTH,
        .datarate = LORA_SPREADING,
        .coding_rate = LORA_CODING_RATE,
        .preamble_len = 8,
        .tx_power = LORA_TX_POWER,
        .tx = true,
    };
    lora_config(lora_dev, &tx_config);

    struct lora_packet pkt = {
        .counter = cnt,
        .type = 'T'
    };

    int ret = lora_send(lora_dev, (uint8_t *)&pkt, sizeof(pkt));
    if (ret == 0) {
        printk("LoRa TX: %u\n", cnt);
    }

    /* RX for echo */
    struct lora_modem_config rx_config = {
        .frequency = LORA_FREQUENCY,
        .bandwidth = LORA_BANDWIDTH,
        .datarate = LORA_SPREADING,
        .coding_rate = LORA_CODING_RATE,
        .preamble_len = 8,
        .tx_power = LORA_TX_POWER,
        .tx = false,
    };
    lora_config(lora_dev, &rx_config);

    uint8_t buf[32];
    int16_t rssi;
    int8_t snr;
    int len = lora_recv(lora_dev, buf, sizeof(buf), K_MSEC(500), &rssi, &snr);
    if (len > 0 && len >= sizeof(struct lora_packet)) {
        struct lora_packet *rx_pkt = (struct lora_packet *)buf;
        if (rx_pkt->type == 'R') {
            printk("LoRa RX: %u | RSSI: %d | SNR: %d\n",
                   rx_pkt->counter, rssi, snr);
        }
    }
}

/* LoRa RX thread disabled - RX is done inline after TX */

int main(void)
{
    printk("\n========================================\n");
    printk("LoRa + BLE Central (TX) - Target1\n");
    printk("========================================\n\n");

    /* Init BLE first */
    printk("Initializing BLE...\n");
    if (bt_enable(NULL)) {
        printk("BLE init failed!\n");
        return 0;
    }
    printk("BLE initialized\n");
    start_scan();

    /* Init LoRa */
    printk("Initializing LoRa...\n");
    lora_dev = LORA_DEV;
    if (lora_init() < 0) {
        printk("LoRa init failed! Continuing with BLE only.\n");
    } else {
        lora_ready = true;
        printk("LoRa ready\n");
    }

    /* Main loop - send counter every second */
    while (1) {
        k_sleep(K_SECONDS(1));

        if (ble_ready) {
            ble_send_counter(counter);
        }

        if (lora_ready) {
            lora_send_counter(counter);
        }

        counter++;
    }

    return 0;
}
