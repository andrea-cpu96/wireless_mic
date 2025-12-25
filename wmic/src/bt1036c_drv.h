/* bt1036c_drv.h - Driver header for BT1036C */
#ifndef BT1036C_DRV_H
#define BT1036C_DRV_H

#include <zephyr/device.h>
#include <stddef.h>
#include <stdint.h>

#define A2DP_SOURCE_PROFILE "339"

#ifdef __cplusplus
extern "C" {
#endif

void bt1036c_config(struct device *uart);
void bt1036c_at_send(const char *cmd);

#ifdef __cplusplus
}
#endif

#endif /* BT1036C_DRV_H */

