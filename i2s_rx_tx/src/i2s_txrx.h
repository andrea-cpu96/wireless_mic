#ifndef I2S_RXTX
#define I2S_RXTX

/* System */
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

/* Support */
#include <math.h>
#include <limits.h>

/* Audio defines */
#define AMP_FACTOR 3
#define MAX_LIMIT (INT32_MAX / (pow(2, AMP_FACTOR)))
#define MIN_LIMIT (INT32_MIN / (pow(2, AMP_FACTOR)))

/* I2S defines */
#define CHANNELS_NUMBER 2
#define SAMPLE_FREQ 20000  // Calculated via script octave
#define I2S_WORD_BYTES 4  // 32 bits word
#define I2S_RX_DELAY 2000 // After this time, i2s_read and i2s_write gives an error

/* Buffer defines */
#define BUFFER_BLOCK_TIME_MS 200
#define DATA_BUFFER_SIZE (SAMPLE_FREQ * BUFFER_BLOCK_TIME_MS / 1000)
#define INITIAL_BLOCKS 2
#define AVAILABLE_BLOCKS 5
#define NUM_BLOCKS (INITIAL_BLOCKS + AVAILABLE_BLOCKS)
#define BLOCK_SIZE (CHANNELS_NUMBER * DATA_BUFFER_SIZE * I2S_WORD_BYTES)

typedef struct
{
    const struct device *dev_i2s;
    struct i2s_config *i2s_cfg;
    uint8_t i2s_cfg_dir;
} i2s_drv_config_t;

int i2s_config(i2s_drv_config_t *hi2s);
int i2s_drv_txrx(i2s_drv_config_t *hi2s);
int i2s_trigger_txrx(i2s_drv_config_t *hi2s);

#endif /* I2S_RXTX */