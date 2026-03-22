#include "audio.h"

/* System */
#include <zephyr/device.h>
/* Standard C libraries */
#include <stdio.h>
/* Debug support */
#include <zephyr/sys/printk.h>

// I2S defines
#define CHANNELS_NUMBER 2
#define SAMPLE_FREQ 44100 // bt module requires 44.1kHz or 48kHz
#define I2S_WORD_BYTES 4  // 32 bits word
#define I2S_RX_DELAY 2000 // After this time, i2s_read and i2s_write gives an error

// Buffer defines
#define BUFFER_BLOCK_TIME_MS 10 // Stored audio track lenght in ms
#define DATA_BUFFER_SIZE ((SAMPLE_FREQ * BUFFER_BLOCK_TIME_MS) / 1000)
#define INITIAL_BLOCKS 2   // Needed by Zephyr I2S driver (>= 2)
#define AVAILABLE_BLOCKS 3 // Further blocks available in case of necessity
#define NUM_BLOCKS (INITIAL_BLOCKS + AVAILABLE_BLOCKS)
#define BLOCK_SIZE (CHANNELS_NUMBER * DATA_BUFFER_SIZE * I2S_WORD_BYTES)

#define AUDIO_TXRX_THREAD_STACK (4096 * 4)
#define AUDIO_TXRX_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(audio_txrx_stack, AUDIO_TXRX_THREAD_STACK);
K_MEM_SLAB_DEFINE(rxtx_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

// Audio data structures
struct audio_handler_t
{
    const struct device *dev_i2s;
    struct i2s_config i2s_cfg;
    uint8_t i2s_cfg_dir;
    pi2s_elab i2s_elab;
} static audio_handler;

static struct k_thread audio_txrx_tcb;

static void audio_txrx_thread(void *a, void *b, void *c);

static int i2s_start_transfer(void);
static int i2s_trigger_txrx(void);
static int i2s_continue_transfer(void);

/**
 * @brief audio_config
 *
 * @param i2s_dev
 * @param cb
 * @return int
 */
int audio_config(const struct device *i2s_dev, pi2s_elab cb)
{
    // Configure I2S
    audio_handler.dev_i2s = i2s_dev;
    audio_handler.i2s_cfg_dir = I2S_DIR_BOTH;
    audio_handler.i2s_elab = cb;

    audio_handler.i2s_cfg.word_size = (I2S_WORD_BYTES * 8);
    audio_handler.i2s_cfg.channels = CHANNELS_NUMBER;
    audio_handler.i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
    audio_handler.i2s_cfg.frame_clk_freq = SAMPLE_FREQ;
    audio_handler.i2s_cfg.block_size = BLOCK_SIZE;
    audio_handler.i2s_cfg.timeout = I2S_RX_DELAY;                                      // This is the max read delay before the i2s_read fails
    audio_handler.i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER; // Set the microcontroller as I2S master and generator of MCK and BCK
    audio_handler.i2s_cfg.mem_slab = &rxtx_mem_slab;

    if (i2s_configure(audio_handler.dev_i2s, audio_handler.i2s_cfg_dir, &audio_handler.i2s_cfg) < 0)
    {
        printf("Failed to configure I2S\n");
        return -1;
    }

    k_thread_create(&audio_txrx_tcb,
                    audio_txrx_stack,
                    AUDIO_TXRX_THREAD_STACK,
                    audio_txrx_thread,
                    NULL, NULL, NULL,
                    AUDIO_TXRX_THREAD_PRIORITY, 0, K_MSEC(500));

    return 0;
}

/**
 * @brief audio_txrx_thread
 *
 * @param a
 * @param b
 * @param c
 */
static void audio_txrx_thread(void *a, void *b, void *c)
{
    i2s_start_transfer();

    // Trigger start
    i2s_trigger_txrx();

    /* Bare metal settings
     *
     *  NOTE1; these settings are not possible with
     *        Zephyr APIs.
     *
     *  NOTE2; these settings must be set after i2s_trigger_txrx() to have effect.
     */
    // NRF_I2S0->CONFIG.MCKFREQ = 0x81F30000; // Calculated via script octave
    NRF_I2S0->CONFIG.RATIO = 6;            // x256
    NRF_I2S0->CONFIG.CLKCONFIG = (0x0101); // Bypass internal MCK scaler (directly take the ACLK source)

    while (1)
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
    if (i2s_trigger(audio_handler.dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_START) < 0)
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

        if (k_mem_slab_alloc(audio_handler.i2s_cfg.mem_slab, &mem_block, K_NO_WAIT) < 0)
        {
            printk("Failed to allocate block\n");
            return -1;
        }

        memset(mem_block, 1, BLOCK_SIZE); // For debug (watch if the line is transmitting 1s)

        if (i2s_write(audio_handler.dev_i2s, mem_block, BLOCK_SIZE) < 0)
        {
            printk("Failed to write block\n");
            return -1;
        }
    }
    printk("Streams started\n");
    return 0;
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

    if (i2s_read(audio_handler.dev_i2s, &mem_block, &block_size) < 0)
    {
        printk("Failed to reaad block\n");
        return -1;
    }

    int32_t *pmem = (int32_t *)mem_block;

    audio_handler.i2s_elab(pmem, block_size);

    if (i2s_write(audio_handler.dev_i2s, mem_block, block_size) < 0)
    {
        printk("Failed to write block\n");
        return -1;
    }

    return 0;
}