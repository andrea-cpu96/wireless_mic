/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
/* Standard C libraries */
#include <stdio.h>
/* Debug support */
#include <zephyr/sys/printk.h>
/* Support */
#include <math.h>

#include "i2s_txrx.h"

#define I2S_DEBUG 0

/* I2S data structures */
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
i2s_drv_config_t hi2s;
static struct i2s_config i2s_cfg_local = {0};

static int i2s_rxtx_init(void);
static int start_transfer(i2s_drv_config_t *hi2s);
static int continue_transfer(i2s_drv_config_t *hi2s);

K_MEM_SLAB_DEFINE(rxtx_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

int main(void)
{
    /* I2S init */
    i2s_rxtx_init();

    k_sleep(K_MSEC(500));

    /* Start the transmission */
    start_transfer(&hi2s); // I2S peripheral needs at least 2 blocks ready before the trigger start

    /* Trigger start*/
    i2s_trigger_txrx(&hi2s);

    while (1)
    {
        /* RX and TX continuosly */
        continue_transfer(&hi2s);

        /* Required time to end the transmission */
        k_sleep(K_MSEC(BUFFER_BLOCK_TIME_MS - 1));
    }

    return 0;
}

/**
 * @brief i2s_rxtx_init
 *
 * @return int
 */
static int i2s_rxtx_init(void)
{
    /* Check device is ready */
    if (!device_is_ready(i2s_dev))
    {
        printf("I2S device not ready\n");
        return -1;
    }

    /* Configure I2S */
    hi2s.dev_i2s = i2s_dev;
    hi2s.i2s_cfg_dir = I2S_DIR_BOTH;
    hi2s.i2s_cfg = &i2s_cfg_local;

    i2s_cfg_local.word_size = I2S_WORD_BYTES * 8;
    i2s_cfg_local.channels = CHANNELS_NUMBER;
    i2s_cfg_local.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_local.frame_clk_freq = SAMPLE_FREQ;
    i2s_cfg_local.block_size = BLOCK_SIZE;
    i2s_cfg_local.timeout = I2S_RX_DELAY; // This is the max read delay before the i2s_read fails
    i2s_cfg_local.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg_local.mem_slab = &rxtx_mem_slab;

    return i2s_config(&hi2s);
}

/**
 * @brief start_transfer; at least 2 blocks must be allocated before triggering the start
 *
 * @param hi2s
 * @return int
 */
static int start_transfer(i2s_drv_config_t *hi2s)
{
    for (int i = 0; i < INITIAL_BLOCKS; ++i)
    {
        void *mem_block;

        if (k_mem_slab_alloc(&rxtx_mem_slab, &mem_block, K_NO_WAIT) < 0)
        {
            printk("Failed to allocate block\n");
            return -1;
        }

        memset(mem_block, 0, BLOCK_SIZE);

        if (i2s_write(hi2s->dev_i2s, mem_block, BLOCK_SIZE) < 0)
        {
            printk("Failed to write block\n");
            return -1;
        }
    }
    printk("Streams started\n");
    return 1;
}

/**
 * @brief continue_transfer
 *
 * @param hi2s
 * @return int
 */
static int continue_transfer(i2s_drv_config_t *hi2s)
{
    i2s_drv_txrx(hi2s);
#if (I2S_DEBUG == 1)
    int free_blocks = k_mem_slab_num_free_get(&rxtx_mem_slab);
    printk("Blocchi slab disponibili: %d\n", free_blocks);
#endif
    return 1;
}