#ifndef BLUETOOTH_DRV_H
#define BLUETOOTH_DRV_H

#include <zephyr/device.h>
#include <stddef.h>
#include <stdint.h>

#define BT103036C_CONFIG_TX 0
#define BT103036C_CONFIG_RX 1

struct bluetooth_peers
{
    char name[100];
    char mac[100];
};

typedef uint16_t (*bt1036c_peers_cb)(const struct bluetooth_peers *peers, const int16_t *size); // Callback function for peers setting

int bluetooth_drv_config(const struct device *uart, bt1036c_peers_cb cb, const uint8_t txrx_config);
void bluetooth_drv_at_send(const char *cmd);

#endif // BLUETOOTH_DRV_H
