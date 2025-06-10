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
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
/* Standard C libraries */
#include <stdio.h>
/* Support */
#include <math.h>

#include "audio.h"
#include "app_peripherals.h"
#include "app_threads.h"

/* Constants */
#define PI (float)3.14159265
#define VOLUME_REF (32768.0 / 10)

/* Tone specs */
#define TONE_FREQ 500
#define DURATION_SEC (float)1.0

/* Audio specs */
#define VOLUME_LEV 5
#define NUM_BLOCKS NUM_OF_BUFFERS // Each block is a buffer (2 buffers; 1 to read and 1 to write simultaneosly + 2 backup buffers)

/* Computations */
#define AMPLITUDE (VOLUME_REF * VOLUME_LEV)
#define SAMPLE_NO (SAMPLE_FREQ / TONE_FREQ)
#define CHUNK_DURATION (float)((float)SAMPLE_NO / (float)SAMPLE_FREQ)
#define NUM_OF_REP (uint16_t)((float)DURATION_SEC / (float)CHUNK_DURATION)

#define BLOCK_SIZE (DATA_BUFFER_SIZE_8) // Buffer size multiplied by the number of bytes per data

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
//K_MEM_SLAB_DEFINE(tx_0_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

/** @brief Sine wave data buffer */
static int16_t sin_data[SAMPLE_NO];
int16_t tx_block[SAMPLE_NO * 2] = {0};

static void generate_sine_wave(void);
static void fill_buf(void);

/**
 * @brief audio_process
 *
 */
void audio_process(void)
{
    adc_config();
    i2s_config();

    adc_thread_create();
    i2s_thread_create();
}

/**
 * @brief i2s_tone
 *
 * @return int
 */
int i2s_tone(void)
{
    /* Generate sine wave */
    generate_sine_wave();
    fill_buf();

    for (int rep = 0; rep < NUM_OF_REP; rep++)
    {
        i2s_drv_send(&hi2s, tx_block, SAMPLE_NO);
    }

    i2s_drv_drain(&hi2s);

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
static void fill_buf(void)
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