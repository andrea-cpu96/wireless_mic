#ifndef I2S_DRV
#define I2S_DRV

/* System */
#include <zephyr/drivers/i2s.h>
#include <zephyr/device.h>

typedef struct
{
    const struct device *dev_i2s;
    struct i2s_config *i2s_cfg;
    uint8_t i2s_cfg_dir;
} i2s_drv_config_t;

int i2s_drv_config(i2s_drv_config_t *i2s_drv);
int i2s_drv_send(i2s_drv_config_t *i2s_drv, void *data, uint32_t num_of_bytes);
int i2s_drv_drop(i2s_drv_config_t *i2s_drv);

#endif /* I2S_DRV */