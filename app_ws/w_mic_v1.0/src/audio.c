#include <stdio.h>
#include <math.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/drivers/i2s.h>

#define PI 3.14159265
#define SAMPLE_NO 3000
#define NUM_BLOCKS 32
#define BLOCK_SIZE (2 * sizeof(data))  // Stereo

/* Sine wave data */
static int16_t data[SAMPLE_NO];

/* Fill sine wave array at 1kHz for 44.1kHz sampling rate */
static void generate_sine_wave(void) {
    float freq = 1000.0f;      // 1kHz
    float sample_rate = 44100.0f;
    for (int i = 0; i < SAMPLE_NO; i++) {
        float angle = 2.0f * PI * freq * ((float)i / sample_rate);
        data[i] = (int16_t)(32767.0f * sinf(angle));
    }
}

/* Fill I2S TX buffer with stereo data */
static void fill_buf(int16_t *tx_block) {
    for (int i = 0; i < SAMPLE_NO; i++) {
        tx_block[2 * i] = data[i];  // Left
        int r_idx = (i + SAMPLE_NO / 4) % SAMPLE_NO;
        tx_block[2 * i + 1] = data[r_idx];  // Right (90° shifted)
    }
}

#ifdef CONFIG_NOCACHE_MEMORY
#define MEM_SLAB_CACHE_ATTR __nocache
#else
#define MEM_SLAB_CACHE_ATTR
#endif

static char MEM_SLAB_CACHE_ATTR __aligned(WB_UP(32))
    _k_mem_slab_buf_tx_0_mem_slab[NUM_BLOCKS * WB_UP(BLOCK_SIZE)];

static STRUCT_SECTION_ITERABLE(k_mem_slab, tx_0_mem_slab) =
    Z_MEM_SLAB_INITIALIZER(tx_0_mem_slab, _k_mem_slab_buf_tx_0_mem_slab,
                           WB_UP(BLOCK_SIZE), NUM_BLOCKS);

int i2s_sample(const struct device *dev_i2s) {
    void *tx_block[NUM_BLOCKS];
    struct i2s_config i2s_cfg = {0};
    int ret;
    uint32_t tx_idx = 0;

    if (!device_is_ready(dev_i2s)) {
        printf("I2S device not ready\n");
        return -ENODEV;
    }

    // Configure I2S
    i2s_cfg.word_size = 16;
    i2s_cfg.channels = 2;
    i2s_cfg.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg.frame_clk_freq = 44100;
    i2s_cfg.block_size = BLOCK_SIZE;
    i2s_cfg.timeout = 2000;
    i2s_cfg.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg.mem_slab = &tx_0_mem_slab;

    ret = i2s_configure(dev_i2s, I2S_DIR_TX, &i2s_cfg);
    if (ret < 0) {
        printf("Failed to configure I2S\n");
        return ret;
    }

    // Generate sine wave
    generate_sine_wave();

    // Prepare blocks
    for (tx_idx = 0; tx_idx < NUM_BLOCKS; tx_idx++) {
        ret = k_mem_slab_alloc(&tx_0_mem_slab, &tx_block[tx_idx], K_FOREVER);
        if (ret < 0) {
            printf("Failed to allocate TX block\n");
            return ret;
        }
        fill_buf((int16_t *)tx_block[tx_idx]);
    }

    tx_idx = 0;
    // Write initial block
    ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
    if (ret < 0) {
        printf("Could not write TX block\n");
        return ret;
    }

    // Start transmission
    ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_START);
    if (ret < 0) {
        printf("Could not trigger I2S\n");
        return ret;
    }

    // Send remaining blocks in loop
    for (int loop = 0; loop < 5; loop++) {
        for (; tx_idx < NUM_BLOCKS; ) {
            ret = i2s_write(dev_i2s, tx_block[tx_idx++], BLOCK_SIZE);
            if (ret < 0) {
                printf("Write failed at block %d\n", tx_idx);
                return ret;
            }
        }
        tx_idx = 0; // loop again
    }

    // Drain
    ret = i2s_trigger(dev_i2s, I2S_DIR_TX, I2S_TRIGGER_DRAIN);
    if (ret < 0) {
        printf("Could not drain I2S\n");
        return ret;
    }

    printf("I2S streaming complete.\n");
    return 0;
}
