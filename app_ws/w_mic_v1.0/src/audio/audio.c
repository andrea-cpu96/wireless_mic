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
#include "app_config.h"
#include "app_peripherals.h"
#include "app_threads.h"

/*********************************************** AUDIO PROCESSING */

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

/*********************************************** SYNTHETIZER */

/* Constants */
#define PI (float)3.14159265
#define VOLUME_REF (32768.0 / 10)

/* Tone specs */
#define TONE_FREQ 500
#define DURATION_SEC (float)1.0

/* Audio specs */
#define VOLUME_LEV 5

/* Computations */
#define AMPLITUDE (VOLUME_REF * VOLUME_LEV)
#define SAMPLE_NO (SAMPLE_FREQ / TONE_FREQ)
#define CHUNK_DURATION (float)((float)SAMPLE_NO / (float)SAMPLE_FREQ)
#define NUM_OF_REP (uint16_t)((float)DURATION_SEC / (float)CHUNK_DURATION)

/** @brief Sine wave data buffer */
static int16_t sin_data[SAMPLE_NO];
int16_t tx_block[SAMPLE_NO * sizeof(int16_t)] = {0};

static void generate_sine_wave(void);
static void fill_buf(void);

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
