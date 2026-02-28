#ifndef I2S_RXTX
#define I2S_RXTX

/* System */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

/* Support */
#include <math.h>
#include <limits.h>

/* I2S defines */
#define CHANNELS_NUMBER 2
#define SAMPLE_FREQ 44100 // bt module requires 44.1kHz or 48kHz
#define I2S_WORD_BYTES 4  // 32 bits word
#define I2S_RX_DELAY 2000 // After this time, i2s_read and i2s_write gives an error

/* Buffer defines */
#define BUFFER_BLOCK_TIME_MS 10
#define DATA_BUFFER_SIZE (SAMPLE_FREQ * BUFFER_BLOCK_TIME_MS / 1000)
#define INITIAL_BLOCKS 2 // Needed by Zephyr I2S driver
#define AVAILABLE_BLOCKS 3 // Further blocks available in case of necessity 
#define NUM_BLOCKS (INITIAL_BLOCKS + AVAILABLE_BLOCKS)
#define BLOCK_SIZE (CHANNELS_NUMBER * DATA_BUFFER_SIZE * I2S_WORD_BYTES)

typedef void (*pi2s_elab)(int32_t *pmem, uint32_t block_size);

typedef struct
{
    const struct device *dev_i2s;
    struct i2s_config *i2s_cfg;
    uint8_t i2s_cfg_dir;
    pi2s_elab i2s_elab;
} i2s_drv_config_t;

int audio_config(i2s_drv_config_t *hi2s);

#endif /* I2S_RXTX */