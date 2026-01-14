/**
 * @file ble_drv.c
 * @brief BLE advertising and connection management for Zephyr-based devices
 *
 * This module provides initialization, advertising, and connection lifecycle management
 * for Bluetooth Low Energy (BLE) peripherals using the Zephyr RTOS.
 * It enables the device to broadcast its presence with custom service UUIDs and device name,
 * and handles automatic restart of advertising after disconnection.
 *
 * Main features:
 * - Initializes the BLE stack and creates a random static address
 * - Advertises with custom flags, device name, and custom 128-bit service UUID
 * - Registers connection callbacks to manage advertising after disconnect
 * - Uses Zephyr workqueue for asynchronous advertising control
 *
 * Functions:
 * - ble_init(): Initializes the Bluetooth LE stack and prepares advertising
 * - ble_start_adv(): Starts advertising as a connectable BLE device
 *
 * Dependencies:
 * - Zephyr kernel and Bluetooth stack
 * - Logging module for runtime diagnostics
 *
 * Usage:
 * Call ble_init() once during system startup, then use ble_start_adv()
 * to begin advertising. Advertising automatically restarts after a connection is closed.
 *
 * @note
 * By default, every BLE device exposes two standard services defined by the Bluetooth SIG:
 * - Generic Access (UUID 0x1800): Handles basic device information and connection parameters.
 * - Generic Attribute (UUID 0x1801): Manages GATT database operations.
 * These services are always present and automatically added by the BLE
 */

#include "ble_drv.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
/* Include the header file of the Bluetooth LE stack */
#include <zephyr/bluetooth/bluetooth.h> // BLE stack
#include <zephyr/bluetooth/gap.h>       // BLE GAP layer
#include <zephyr/bluetooth/uuid.h>      // BLE UUIDs
#include <zephyr/bluetooth/addr.h>      // BLE address types
#include <zephyr/bluetooth/conn.h>      // BLE connection

LOG_MODULE_REGISTER(ble_logs, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/**
 * @brief work element
 *
 */
static struct k_work adv_work;

/**
 * @brief Bluetooth connection element
 */
struct bt_conn *my_conn = NULL;

/**
 * @brief Advertising data
 *
 * @note in BT_DATA_BYTES after type, each entry is a single byte
 * @note only 31 octects available
 */
static const struct bt_data ad[] = {
    /* Set the advertising flags */
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)), // BT_LE_AD_GENERAL enable the discoverable mode
    /* Set the advertising packet data */
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/**
 * @brief scan response data
 *
 */
static const struct bt_data sd[] = {
    /* Set the UUID (identifies the service) */
    BT_DATA_BYTES(
        BT_DATA_UUID128_ALL,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0), // Custum service UUID
};

static void adv_work_handler(struct k_work *work);
static void advertising_start(void);
static void recycled_cb(void);
static void on_connected_cb(struct bt_conn *conn, uint8_t err);
static void on_disconnected_cb(struct bt_conn *conn, uint8_t reason);

/**
 * @brief assign callback functions to bluetooth events
 *
 */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = on_connected_cb,       // post bt connection callback
    .disconnected = on_disconnected_cb, // post bt disconnection callback
    .recycled = recycled_cb,            // post bt disconnection callback
};

/**
 * @brief ble_init
 *
 * @return int
 */
int ble_init(void)
{
    bt_addr_le_t addr;
    int err;

    /* Create random static address */

    err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
    if (err)
    {
        printk("Invalid BT address (err %d)\n", err);
        return -1;
    }

    err = bt_id_create(&addr, NULL);
    if (err < 0)
    {
        printk("Creating new ID failed (err %d)\n", err);
        return -1;
    }

    LOG_INF("Bluetooth initialized\n");

    /* Initialize adv work element */

    k_work_init(&adv_work, adv_work_handler);

    /* Enable the BLE stack */

    err = bt_enable(NULL); // Blocking mode (no callback passed)
    if (err)
    {
        LOG_ERR("Bluetooth init failed (err %d)\n", err);
        return -1;
    }

    return 1;
}

/**
 * @brief ble_start_adv
 *
 */
void ble_start_adv(void)
{
    /* Start advertising */

    advertising_start();

    return;
}

/**
 * @brief advertising_start
 *
 */
static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

/**
 * @brief restart advertising after disconnection
 *
 * @param work
 */
static void adv_work_handler(struct k_work *work)
{
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd)); // Connectable (BT_LE_ADV_CONN_FAST_1), no scan response sent (sd = NULL)
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)\n", err);
        return;
    }
}

/**
 * @brief recycled_cb
 *
 */
static void recycled_cb(void)
{
    LOG_INF("Connection object available from previous conn. Disconnect is complete!\n");
    advertising_start();
}

/**
 * @brief on_connected_cb
 */
static void on_connected_cb(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_ERR("Connection error %d", err);
        return;
    }
    LOG_INF("Connected");
    my_conn = bt_conn_ref(conn);
}

/**
 * @brief on_disconnected_cb
 */
static void on_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected. Reason %d", reason);
    bt_conn_unref(my_conn);
}
