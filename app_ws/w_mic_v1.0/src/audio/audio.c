/**
 * @file audio.c
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides a library audio applicaitons.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

/* System */
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/i2s.h>
/* Standard C libraries */
#include <stdio.h>
/* Support */
#include <math.h>

#include "audio.h"

/* Constants */
#define PI (float)3.14159265
#define VOLUME_REF (32768.0 / 10)

/* Tone specs */
#define TONE_FREQ 500
#define DURATION_SEC (float)3.0

/* Audio specs */
#define STEREO 1 // MAX98357A always exspects input data in stereo format (it selects the channnel via  SD pin)
#define VOLUME_LEV 5
#define SAMPLE_FREQ 44100
#define NUM_BLOCKS 4 // Each block is a buffer (2 buffers; 1 to read and 1 to write simultaneosly + 2 backup buffers)
#if (STEREO == 1)
#define CHANNELS_NUMBER 2
#else
#define CHANNELS_NUMBER 1
#endif

/* Computations */
#define AMPLITUDE (VOLUME_REF * VOLUME_LEV)
#define SAMPLE_NO (SAMPLE_FREQ / TONE_FREQ)
#define CHUNK_DURATION (float)((float)SAMPLE_NO * (float)NUM_BLOCKS / (float)SAMPLE_FREQ)
#define NUM_OF_REP (uint16_t)((float)DURATION_SEC / (float)CHUNK_DURATION)

/** @brief Sine wave data buffer */
static int16_t sin_data[SAMPLE_NO];

#define BLOCK_SIZE (CHANNELS_NUMBER * sizeof(sin_data))

/** @brief Slab memory structure
 *
 * A slab memory structure organizes data into blocks
 * of memory with the same size and aligned.
 *
 * The memory is then accessed per block (not single data)
 * This means;
 * 1) More blocks that are smaller → higher granularity, lower audio delay,
 *    but increased overhead due to more frequent accesses to different blocks.
 * 2) Fewer blocks that are bigger → lower overhead (fewer memory accesses,
 *    potentially reducing distortion), but increased audio delay since more
 *    time is needed to wait for a block to become available for new data.
 *
 * The main advantages of this data structure are;
 * 1) Deterministic memory access to the data
 * 2) No memory fragmentation of the data
 */
K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

static void generate_sine_wave(void);
static void fill_buf(int16_t *tx_block);

void *tx_block[NUM_BLOCKS] = {0}; // Pointer to the blocks

/**
 * @brief I2S configuration
 *
 * @param dev_i2s
 * @return int
 */
int i2s_config(const struct device *dev_i2s)
{
    struct i2s_config i2s_cfg = {0};

    /* Check device is ready */
    if (!device_is_ready(dev_i2s))
    {
        printf("I2S device not ready\n");
        return -1;
    }

    /* Configure I2S */
    i2s_cfg.word_size = 16;
    i2s_cfg.channels = CHANNELS_NUMBER;
    i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg.frame_clk_freq = SAMPLE_FREQ;
    i2s_cfg.block_size = BLOCK_SIZE;
    i2s_cfg.timeout = 2000;
    i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg.mem_slab = &tx_0_mem_slab;

    if (i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg) < 0)
    {
        printf("Failed to configure I2S\n");
        return -1;
    }

    return 0;
}

/**
 * @brief i2s example
 *
 * @param dev_i2s
 * @return int
 */
int i2s_sample(const struct device *dev_i2s)
{
    volatile uint32_t tx_idx = 0;

    /* Generate sine wave */
    generate_sine_wave();

    /* Allocate slab blocks */
    for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++)
    {
        /* One block allocated for each cycle */
        if (k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER) < 0)
        {
            printf("Failed to allocate TX block\n");
            return -1;
        }
        /* Fill each block with data */
        fill_buf((int16_t *)tx_block[tx_idx]);
    }

    /* Write initial block (needed by I2S to know at least one block is ready)*/
    if (i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE) < 0)
    {
        printf("Could not write TX block\n");
        return -1;
    }

    /* Start transmission */
    if (i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START) < 0)
    {
        printf("Could not trigger I2S\n");
        return -1;
    }

    /* Send remaining blocks in loop */
    uint16_t rep_num = NUM_OF_REP;
    for (int loop = 0; loop < rep_num; loop++)
    {
        for (; tx_idx < NUM_BLOCKS;)
        {
            if (i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE) < 0)
            {
                printf("Write failed at block %d\n", tx_idx);
                return -1;
            }
        }
        tx_idx = 0; // loop again
    }

    /* Drain (kill the I2S communication)*/
    if (i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN) < 0)
    {
        printf("Could not drain I2S\n");
        return -1;
    }

    printf("I2S streaming complete.\n");
    return 0;
}

/**
 * @brief Create the sinusoidal wave
 */
static void generate_sine_wave(void)
{
    float freq = (float)TONE_FREQ;
    float sample_rate = (float)SAMPLE_FREQ;

    memset(sin_data, 0, sizeof(sin_data));
    for (int i = 0; i < SAMPLE_NO; i++)
    {
        float angle = 2.0f * PI * freq * ((float)i / sample_rate);
        sin_data[i] = (int16_t)((float)AMPLITUDE * sinf(angle));
    }
}

/**
 * @brief Fill I2S TX buffer with stereo data
 *
 * This code also simulates a stereo signal by shifting
 * the right channel with the signal delayed by 90 degree.
 */
static void fill_buf(int16_t *tx_block)
{
    for (int i = 0; i < SAMPLE_NO; i++)
    {
#if (STEREO == 1)
        tx_block[2 * i] = sin_data[i]; // Left channel
        tx_block[2 * i + 1] = 0;       // Right channel
#if (0)
        /* Fake right channel */
        int r_idx = (i + SAMPLE_NO / 4) % SAMPLE_NO;
        tx_block[2 * i + 1] = sin_data[r_idx]; // Right channel (90° shifted)
#endif
#else
        tx_block[i] = sin_data[i]; // Left channel
#endif
    }
}