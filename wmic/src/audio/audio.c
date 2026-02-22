#include "audio.h"

/* System */
#include <zephyr/device.h>
/* Standard C libraries */
#include <stdio.h>
/* Debug support */
#include <zephyr/sys/printk.h>

#define AUDIO_TXRX_THREAD_STACK (4096*4) 
K_THREAD_STACK_DEFINE(audio_txrx_stack, AUDIO_TXRX_THREAD_STACK);

i2s_drv_config_t *hi2s_int;

static struct k_thread audio_txrx_tcb;

static void audio_txrx_thread(void *a, void *b, void *c);

static int i2s_start_transfer(void);
static int i2s_trigger_txrx(void);
static int i2s_continue_transfer(void);

/**
 * @brief i2s_config
 *
 * @param hi2s
 * @return int
 */
int audio_config(i2s_drv_config_t *hi2s)
{
    if (i2s_configure(hi2s->dev_i2s, hi2s->i2s_cfg_dir, hi2s->i2s_cfg) < 0)
    {
        printf("Failed to configure I2S\n");
        return -1;
    }

    hi2s_int = hi2s;

    k_thread_create(&audio_txrx_tcb,
                    audio_txrx_stack,
                    AUDIO_TXRX_THREAD_STACK,
                    audio_txrx_thread,
                    NULL, NULL, NULL,
                    5, 0, K_MSEC(500));

    return 0;
}

static void audio_txrx_thread(void *a, void *b, void *c)
{
    i2s_start_transfer();

    /* Trigger start*/
    i2s_trigger_txrx();

    /* Bare metal settings
     *
     *  NOTE1; these settings are not possible with
     *        Zephyr APIs.
     *
     *  NOTE2; these settings must be set after i2s_trigger_txrx() to have effect.
     */
    //NRF_I2S0->CONFIG.MCKFREQ = 0x81F30000; // Calculated via script octave
    NRF_I2S0->CONFIG.RATIO = 6;            // x256
    NRF_I2S0->CONFIG.CLKCONFIG = (0x0101);
    while(1)
    {
        // Thread is blocked by i2s_read untill new data arrive
        i2s_continue_transfer();
    }
}

/**
 * @brief i2s_trigger_txrx
 *
 * @param none
 * @return int
 */
static int i2s_trigger_txrx(void)
{
    if (i2s_trigger(hi2s_int->dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START) < 0)
    {
        printf("Could not trigger TX I2S\n");
        return -1;
    }
    return 0;
}

/**
 * @brief i2s_start_transfer; at least 2 blocks must be allocated before triggering the start
 *
 * @param none
 * @return int
 */
static int i2s_start_transfer(void)
{
    for (int i = 0; i < INITIAL_BLOCKS; ++i)
    {
        void *mem_block;

        if (k_mem_slab_alloc(hi2s_int->i2s_cfg->mem_slab, &mem_block, K_NO_WAIT) < 0)
        {
            printk("Failed to allocate block\n");
            return -1;
        }

        memset(mem_block, 1, BLOCK_SIZE);

        if (i2s_write(hi2s_int->dev_i2s, mem_block, BLOCK_SIZE) < 0)
        {
            printk("Failed to write block\n");
            return -1;
        }
    }
    printk("Streams started\n");
    return 1;
}

/**
 * @brief i2s_continue_transfer
 *
 * @param none
 * @return int
 */
static int i2s_continue_transfer(void)
{
    void *mem_block;
    uint32_t block_size;

    if (i2s_read(hi2s_int->dev_i2s, &mem_block, &block_size) < 0)
        return -1;

    int32_t *pmem = (int32_t *)mem_block;
    
    hi2s_int->i2s_elab(pmem, block_size);

    if (i2s_write(hi2s_int->dev_i2s, mem_block, block_size) < 0)
        return -1;

    return 0;
}