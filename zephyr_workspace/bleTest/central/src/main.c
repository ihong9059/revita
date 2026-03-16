/*
 * BLE Central (TX) - Target1
 * - Scans for "REVITA_RX"
 * - Connects and sends counter every 1 second
 * - Receives echo notification
 * - Displays RSSI and TX Power
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

/* Custom Service UUID */
static struct bt_uuid_128 revita_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

/* RX Characteristic UUID (Write) */
static struct bt_uuid_128 revita_rx_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

/* TX Characteristic UUID (Notify) */
static struct bt_uuid_128 revita_tx_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

static struct bt_conn *current_conn;
static uint16_t rx_handle;      /* Write handle */
static uint16_t tx_handle;      /* Notify handle */
static uint16_t ccc_handle;     /* CCC handle */
static bool handles_found;
static bool subscribed;
static uint32_t counter;
static int8_t current_rssi;
static const int8_t tx_power_level = 8;

/* Forward declarations */
static void start_scan(void);

/* Read RSSI */
static void read_conn_rssi(void)
{
    if (!current_conn) return;

    uint16_t handle;
    int err = bt_hci_get_conn_handle(current_conn, &handle);
    if (err) return;

    struct bt_hci_cp_read_rssi *cp;
    struct bt_hci_rp_read_rssi *rp;
    struct net_buf *buf, *rsp = NULL;

    buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
    if (!buf) return;

    cp = net_buf_add(buf, sizeof(*cp));
    cp->handle = sys_cpu_to_le16(handle);

    err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
    if (err) return;

    rp = (void *)rsp->data;
    current_rssi = rp->rssi;
    net_buf_unref(rsp);
}

/* Notification callback */
static uint8_t notify_cb(struct bt_conn *conn,
                         struct bt_gatt_subscribe_params *params,
                         const void *data, uint16_t length)
{
    if (!data) {
        printk("Unsubscribed\n");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    if (length >= sizeof(uint32_t)) {
        uint32_t received;
        memcpy(&received, data, sizeof(received));
        read_conn_rssi();
        printk("RX: %u | RSSI: %d dBm | TX Power: +%d dBm\n",
               received, current_rssi, tx_power_level);
    }

    return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_subscribe_params subscribe_params;

static void subscribe_to_tx(void)
{
    int err;

    subscribe_params.notify = notify_cb;
    subscribe_params.value_handle = tx_handle;
    subscribe_params.ccc_handle = ccc_handle;
    subscribe_params.value = BT_GATT_CCC_NOTIFY;

    err = bt_gatt_subscribe(current_conn, &subscribe_params);
    if (err) {
        printk("Subscribe failed (err %d)\n", err);
    } else {
        printk("Subscribed to notifications\n");
        subscribed = true;
    }
}

/* Discovery state */
static uint16_t service_start_handle;
static uint16_t service_end_handle;
static struct bt_gatt_discover_params discover_params;

/* CCC discovery callback */
static uint8_t ccc_discover_cb(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               struct bt_gatt_discover_params *params)
{
    if (!attr) {
        printk("CCC discovery complete\n");
        if (rx_handle && tx_handle && ccc_handle) {
            handles_found = true;
            printk("All handles found: rx=%u, tx=%u, ccc=%u\n",
                   rx_handle, tx_handle, ccc_handle);
            k_sleep(K_MSEC(100));
            subscribe_to_tx();
        }
        return BT_GATT_ITER_STOP;
    }

    printk("  CCC handle: %u\n", attr->handle);
    ccc_handle = attr->handle;

    return BT_GATT_ITER_CONTINUE;
}

/* Characteristic discovery callback */
static uint8_t char_discover_cb(struct bt_conn *conn,
                                const struct bt_gatt_attr *attr,
                                struct bt_gatt_discover_params *params)
{
    if (!attr) {
        printk("Char discovery complete\n");
        /* CCC is right after TX characteristic value */
        if (rx_handle && tx_handle) {
            ccc_handle = tx_handle + 1;
            handles_found = true;
            printk("Handles: rx=%u, tx=%u, ccc=%u\n", rx_handle, tx_handle, ccc_handle);
            k_sleep(K_MSEC(100));
            subscribe_to_tx();
        }
        return BT_GATT_ITER_STOP;
    }

    struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

    if (bt_uuid_cmp(chrc->uuid, &revita_rx_uuid.uuid) == 0) {
        rx_handle = chrc->value_handle;
        printk("  RX handle: %u\n", rx_handle);
    } else if (bt_uuid_cmp(chrc->uuid, &revita_tx_uuid.uuid) == 0) {
        tx_handle = chrc->value_handle;
        printk("  TX handle: %u\n", tx_handle);
    }

    return BT_GATT_ITER_CONTINUE;
}

/* Service discovery callback */
static uint8_t svc_discover_cb(struct bt_conn *conn,
                               const struct bt_gatt_attr *attr,
                               struct bt_gatt_discover_params *params)
{
    if (!attr) {
        printk("Service discovery complete\n");
        if (service_start_handle > 0) {
            printk("Discovering characteristics...\n");
            discover_params.uuid = NULL;
            discover_params.func = char_discover_cb;
            discover_params.start_handle = service_start_handle;
            discover_params.end_handle = service_end_handle;
            discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
            k_sleep(K_MSEC(100));
            bt_gatt_discover(conn, &discover_params);
        }
        return BT_GATT_ITER_STOP;
    }

    struct bt_gatt_service_val *svc = (struct bt_gatt_service_val *)attr->user_data;

    if (bt_uuid_cmp(svc->uuid, &revita_service_uuid.uuid) == 0) {
        service_start_handle = attr->handle;
        service_end_handle = svc->end_handle;
        printk("REVITA Service found [%u-%u]\n",
               service_start_handle, service_end_handle);
    }

    return BT_GATT_ITER_CONTINUE;
}

static void discover_services(struct bt_conn *conn)
{
    printk("Discovering services...\n");

    service_start_handle = 0;
    service_end_handle = 0;
    rx_handle = 0;
    tx_handle = 0;
    ccc_handle = 0;

    discover_params.uuid = NULL;
    discover_params.func = svc_discover_cb;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    int err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        printk("Service discovery failed (err %d)\n", err);
    }
}

/* Scan callback */
static void device_found(const bt_addr_le_t *addr, int8_t rssi,
                         uint8_t type, struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];

    if (current_conn) return;

    struct net_buf_simple_state state;
    net_buf_simple_save(ad, &state);

    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        uint8_t ad_type;

        if (len == 0 || len > ad->len) break;

        ad_type = net_buf_simple_pull_u8(ad);
        len--;

        if ((ad_type == BT_DATA_NAME_COMPLETE || ad_type == BT_DATA_NAME_SHORTENED) &&
            len == 9 && memcmp(ad->data, "REVITA_RX", 9) == 0) {
            bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
            printk("Found REVITA_RX [%s] RSSI:%d\n", addr_str, rssi);

            bt_le_scan_stop();

            struct bt_conn_le_create_param create_param = BT_CONN_LE_CREATE_PARAM_INIT(
                BT_CONN_LE_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
            struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(80, 80, 0, 400);

            int err = bt_conn_le_create(addr, &create_param, &conn_param, &current_conn);
            if (err) {
                printk("Connect failed (err %d)\n", err);
                start_scan();
            }
            return;
        }
        net_buf_simple_pull(ad, len);
    }
    net_buf_simple_restore(ad, &state);
}

static void start_scan(void)
{
    struct bt_le_scan_param scan_param = {
        .type = BT_LE_SCAN_TYPE_ACTIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    int err = bt_le_scan_start(&scan_param, device_found);
    if (err) {
        printk("Scan failed (err %d)\n", err);
        return;
    }
    printk("Scanning...\n");
}

/* Send counter */
static void send_counter(void)
{
    if (!current_conn || !handles_found || !subscribed) return;

    int err = bt_gatt_write_without_response(current_conn, rx_handle,
                                              &counter, sizeof(counter), false);
    if (err) {
        printk("Write failed (err %d)\n", err);
    } else {
        printk("TX: %u\n", counter);
        counter++;
    }
}

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err 0x%02x)\n", err);
        if (current_conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;
        }
        start_scan();
        return;
    }

    printk("Connected!\n");
    handles_found = false;
    subscribed = false;
    k_sleep(K_MSEC(500));
    discover_services(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (0x%02x)\n", reason);

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    handles_found = false;
    subscribed = false;
    start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int main(void)
{
    printk("\n=================================\n");
    printk("BLE Central (TX) - Target1\n");
    printk("TX Power: +%d dBm\n", tx_power_level);
    printk("=================================\n\n");

    int err = bt_enable(NULL);
    if (err) {
        printk("BT init failed (err %d)\n", err);
        return 0;
    }

    printk("Bluetooth initialized\n");
    start_scan();

    while (1) {
        k_sleep(K_SECONDS(1));
        if (current_conn && handles_found && subscribed) {
            send_counter();
        }
    }

    return 0;
}
