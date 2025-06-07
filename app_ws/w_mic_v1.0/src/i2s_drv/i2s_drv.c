/* System */
#include <zephyr/kernel.h>
/* Standard C libraries */
#include <stdio.h>
/* Support */
#include <math.h>

#include "i2s_drv.h"

/**
 * @brief I2S configuration
 *
 * @param i2s_drv_config_t
 * @return int
 */
int i2s_drv_config(i2s_drv_config_t *i2s_drv)
{
    /* Check device is ready */
    if (!device_is_ready(i2s_drv->dev_i2s))
    {
        printf("I2S device not ready\n");
        return -1;
    }

    if (i2s_configure(i2s_drv->dev_i2s, i2s_drv->i2s_cfg_dir, i2s_drv->i2s_cfg) < 0)
    {
        printf("Failed to configure I2S\n");
        return -1;
    }

    /* Slab memory configuration */

    /*
     * mem_slab contains metadata related to the slab memory structure.
     * buffer defines the address of the slab memory.
     * block_num can be thought as the number of sub-buffers.
     *
     * Blocks can then be allocated by the buffer when required.
     */
    return 0;
}

/**
 * @brief i2s sending of one block
 *
 * @param dev_i2s
 * @return int
 */
int i2s_drv_send(i2s_drv_config_t *i2s_drv, void *data, uint32_t num_of_bytes)
{
    uint32_t *tx_block = NULL;
    uint32_t blocks_to_send = (uint32_t)ceil((float)num_of_bytes / (float)i2s_drv->i2s_cfg->block_size);
    static bool first_iter = true;

    uint32_t *data_start_addr = (uint32_t *)data;

    data = data_start_addr;
    for (; blocks_to_send > 0; blocks_to_send--)
    {
        /* Allocate one slab block */

        /*
         * tx_block will point to the block assigned.
         */
        if (k_mem_slab_alloc(i2s_drv->i2s_cfg->mem_slab, (void **)&tx_block, K_FOREVER) < 0)
        {
            printf("Failed to allocate TX block\n");
            return -1;
        }
        memcpy(tx_block, data, i2s_drv->i2s_cfg->block_size);
        data = (uint8_t *)data + i2s_drv->i2s_cfg->block_size;

        /* Write data over I2S queue */

        /*
         * This function takes ownership of the memory block and will release it when all data are transmitted.
         */
        if (i2s_write(i2s_drv->dev_i2s, tx_block, i2s_drv->i2s_cfg->block_size) < 0)
        {
            printf("Could not write TX block\n");
            return -1;
        }

        if (first_iter)
        {
            first_iter = false;

            /* Start transmission */
            if (i2s_trigger(i2s_drv->dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START) < 0)
            {
                printf("Could not trigger I2S\n");
                return -1;
            }
        }
    }

    return 0;
}

int i2s_drv_drain(i2s_drv_config_t *i2s_drv)
{
    /* Finish current block and then stop */
    if (i2s_trigger(i2s_drv->dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN) < 0)
    {
        printf("Could not drain I2S\n");
        return -1;
    }
    
    return 0;
}