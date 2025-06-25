/* System */
#include <zephyr/device.h>
/* Standard C libraries */
#include <stdio.h>
/* Debug support */
#include <zephyr/sys/printk.h>

#include "i2s_txrx.h"

/**
 * @brief i2s_config
 *
 * @return int
 */
int i2s_config(i2s_drv_config_t *hi2s)
{
    if (i2s_configure(hi2s->dev_i2s, hi2s->i2s_cfg_dir, hi2s->i2s_cfg) < 0)
    { 
        printf("Failed to configure I2S\n");
        return -1;
    }
    return 0;
}

/**
 * @brief i2s_drv_write
 *
 * @return int
 */
int i2s_drv_txrx(i2s_drv_config_t *hi2s)
{
    void *mem_block;
    uint32_t block_size;

    if (i2s_read(hi2s->dev_i2s, &mem_block, &block_size) < 0)
    {
        printk("Failed to read data\n");
        return -1;
    }

    if (i2s_write(hi2s->dev_i2s, mem_block, block_size) < 0)
    {
        printk("Failed to write data\n");
        return -1;
    }

    return 0;
}

/**
 * @brief i2s_trigger_txrx
 *
 * @param hi2s
 * @return int
 */
int i2s_trigger_txrx(i2s_drv_config_t *hi2s)
{
    if (i2s_trigger(hi2s->dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START) < 0)
    {
        printf("Could not trigger TX I2S\n");
        return -1;
    }
    return 0;
}