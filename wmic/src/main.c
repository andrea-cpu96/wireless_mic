/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
/* Standard C libraries */
#include <stdio.h>
/* Debug support */
#include <zephyr/sys/printk.h>

#include "config.h"
#include "i2s_txrx.h"
#include "ble_drv.h"

//#include "sw_codec_lc3.h"

/* LED data structures */
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

/* I2S data structures */
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
i2s_drv_config_t hi2s;
static struct i2s_config i2s_cfg_local = {0};

/* Callback functions */
static void data_elab_cb(int32_t *pmem, uint32_t block_size);

/* Init functions */
static int led_init(void);
static int audio_init(void);
static int i2s_rxtx_init(void);

/* Streaming functions */
static int start_transfer(i2s_drv_config_t *hi2s);
static int continue_transfer(i2s_drv_config_t *hi2s);

/* Error handler function */
static void err_handler(void);

K_MEM_SLAB_DEFINE(rxtx_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

int main(void)
{
    /* LED init */
    if (led_init() < 0)
        err_handler();

    /* Audio init */
    if (audio_init() < 0)
        err_handler();

    /* I2S init */
    if (i2s_rxtx_init() < 0)
        err_handler();

    /* BLE init */
    if (ble_init() < 0)
        err_handler();

    k_sleep(K_MSEC(500));

    /* Start peripheral advertising */
    ble_start_adv();

    /* Signal advertising is started */
    gpio_pin_set(led.port, led.pin, 1);

    /* Start the transmission */
    start_transfer(&hi2s); // I2S peripheral needs at least 2 blocks ready before the trigger start

    /* Trigger start*/
    i2s_trigger_txrx(&hi2s);

    /* Bare metal settings
     *
     *  NOTE1; these settings are not possible with
     *        Zephyr APIs.
     *
     *  NOTE2; these settings must be set after i2s_trigger_txrx() to have effect.
     */
    NRF_I2S0->CONFIG.MCKFREQ = 0x6318C000; // Calculated via script octave
    NRF_I2S0->CONFIG.RATIO = 7;            // 384 (stick to octave calculations)

    while (1)
    {
        /* RX and TX continuosly */
        continue_transfer(&hi2s);
    }
    return 0;
}

/**
 * @brief led_init
 *
 * @return int
 */
static int led_init(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    return 1;
}

static int audio_init(void)
{
 //   static struct sw_codec_config _sw_codec_config;
    int ret = 0;
/*
    _sw_codec_config.sw_codec = SW_CODEC_LC3;

    _sw_codec_config.encoder.bitrate = CONFIG_LC3_MONO_BITRATE;
    _sw_codec_config.encoder.channel_mode = SW_CODEC_STEREO;
    _sw_codec_config.encoder.enabled = true;

    ret = sw_codec_init(_sw_codec_config);

    _sw_codec_config.initialized = true;
*/
    return ret;
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
    hi2s.i2s_elab = data_elab_cb;

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

        memset(mem_block, 1, BLOCK_SIZE);

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

static void data_elab_cb(int32_t *pmem, uint32_t block_size)
{
    int size = block_size / sizeof(int32_t);

    const float max = MAX_LIMIT;
    const float min = MIN_LIMIT;

    for (int i = 0; i < size; i++)
    {
        if ((pmem[i] <= max) && (pmem[i] >= min))
        {
            pmem[i] <<= AMP_FACTOR;
        }
        else
        {
            pmem[i] = (pmem[i] >= 0) ? INT32_MAX : INT32_MIN;
        }
    }
}

static void err_handler(void)
{
    while (1)
        ;
}