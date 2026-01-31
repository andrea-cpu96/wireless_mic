/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
/* Standard C libraries */
#include <stdio.h>
#if (ENABLE_DSP_FILTER)
#include <arm_math.h>
#endif // ENABLE_DSP_FILTER
/* Project modules */
#include "config.h"
#include "audio.h"
#include "bt1036c_drv.h"
#if (ENABLE_DSP_FILTER)
#include "low_pass_filter.h"
#endif // ENABLE_DSP_FILTER
#include "signals.h"

K_MEM_SLAB_DEFINE(rxtx_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

/* LED data structures */
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

/* I2S data structures */
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
i2s_drv_config_t hi2s;
static struct i2s_config i2s_cfg_local = {0};

/* UART data structures */
const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

static int gpios_init(void);
static int uart_init(void);

static int audio_init(void);
static void bt_init(void);

#if (ENABLE_DSP_FILTER)
static void dsp_filter_init();
static void dsp_filter(int32_t *pmem, uint32_t size);
#endif // ENABLE_DSP_FILTER

static void data_elab(int32_t *pmem, uint32_t block_size);

const float max = MAX_LIMIT;
const float min = MIN_LIMIT;

int main(void)
{
    /* GPIOS init */
    gpios_init();

    /* UART init */
    uart_init();

    /* Filter init */
#if (ENABLE_DSP_FILTER)
    dsp_filter_init();
#endif // ENABLE_DSP_FILTER

    /* Bluetooth init */
    bt_init();

    /* Audio init */
    audio_init();

    k_sleep(K_MSEC(500));

    /* App is running */
    gpio_pin_set(led.port, led.pin, 1);

    while (1)
    {
        k_sleep(K_MSEC(500));
    }
    return 0;
}

/**
 * @brief audio_init
 *
 * @return int
 */
static int audio_init(void)
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
    hi2s.i2s_elab = data_elab;

    i2s_cfg_local.word_size = I2S_WORD_BYTES * 8;
    i2s_cfg_local.channels = CHANNELS_NUMBER;
    i2s_cfg_local.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_local.frame_clk_freq = SAMPLE_FREQ;
    i2s_cfg_local.block_size = BLOCK_SIZE;
    i2s_cfg_local.timeout = I2S_RX_DELAY;                                      // This is the max read delay before the i2s_read fails
    i2s_cfg_local.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER; // Set the microcontroller as I2S master and generator of MCK and BCK
    i2s_cfg_local.mem_slab = &rxtx_mem_slab;

    return audio_config(&hi2s);
}

/**
 * @brief uart_init
 *
 * @return int
 */
static int uart_init(void)
{
    /* Check device is ready */
    if (!device_is_ready(uart0_dev))
    {
        printf("UART device not ready\n");
        return -1;
    }

    return 0;
}

/**
 * @brief bt_init
 *
 * @return void
 */
static void bt_init(void)
{
    bt1036c_config(uart0_dev, TXRX_MODULE);
}

/**
 * @brief gpios_init
 *
 * @return int
 */
static int gpios_init(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    return 1;
}

/**
 * @brief data_elab
 *
 * @return void
 */
static void data_elab(int32_t *pmem, uint32_t block_size)
{
    int size = block_size / sizeof(int32_t);

#if (ENABLE_SIGNAL_GEN)
    for (int i = 0; i < size; i = i + 2)
    {
        int index = (i / 2) % SIG_GEN_LEN;
        pmem[i] = (int32_t)(signals_get_sample(index) * (float32_t)32767);
        pmem[i] = (pmem[i] << 16);
        pmem[i + 1] = pmem[i];
    }
#endif // ENABLE_SIGNAL_GEN

#if (ENABLE_DSP_FILTER)
    dsp_filter(pmem, size);
#endif // ENABLE_DSP_FILTER

#if (ENABLE_STEREO_DIFF)
    for (int i = 0; i < size; i += 2)
    {
        int32_t diff = pmem[i + 1] - pmem[i]; // right - left
        pmem[i] = diff;
        pmem[i + 1] = diff;
    }
#endif // ENABLE_STEREO_DIFF

#if (!ENABLE_SIGNAL_GEN)
    for (int i = 0; i < size; i = i + 2)
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
#endif // !ENABLE_SIGNAL_GEN
}

#if (ENABLE_DSP_FILTER)
/**
 * @brief dsp_filter_init
 *
 * @return void
 */
static void dsp_filter_init(void)
{ 
    lowpass_filter_init(1); // block_len = 1
    return;
}

/**
 * @brief dsp_filter
 *
 * @return void
 */
static void dsp_filter(int32_t *pmem, uint32_t size)
{
    float32_t data_f32 = 0.0;
    q15_t data_q15;
    q15_t out;

#if (ENABLE_STEREO_DIFF)
    for (int i = 0; i < size; i += 2)
    {
        data_f32 = ((pmem[i]) / (float32_t)2147483648);
        arm_float_to_q15(&data_f32, &data_q15, 1);
        lowpass_filter_exc(&data_q15, &out);
        int32_t filtered = (int32_t)(out * (2147483648 / 32768) * 10);
        pmem[i] = filtered;     // Left channel
        pmem[i + 1] = filtered; // Right channel
    }
#else
    // Left channel
    for (int i = 0; i < size; i += 2)
    {
        data_f32 = ((pmem[i]) / (float32_t)2147483648);
        arm_float_to_q15(&data_f32, &data_q15, 1);
        lowpass_filter_exc(&data_q15, &out);
        pmem[i] = (int32_t)(out * (2147483648 / 32768) * 10);
    }

    // Right channel
    for (int i = 1; i < size; i += 2)
    {
        data_f32 = ((pmem[i]) / (float32_t)2147483648);
        arm_float_to_q15(&data_f32, &data_q15, 1);
        lowpass_filter_exc(&data_q15, &out);
        pmem[i] = (int32_t)(out * (2147483648 / 32768) * 10);
    }
#endif // ENABLE_STEREO_DIFF

    return;
}
#endif // ENABLE_DSP_FILTER