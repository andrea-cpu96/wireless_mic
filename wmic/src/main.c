/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h> 
/* Standard C libraries */
#include <stdio.h>
#include <arm_math.h>
/* Project modules */
#include "config.h"
#include "i2s_txrx.h"
#include "ble_drv.h"
#include "low_pass_filter.h"
#include "bt1036c_drv.h"

#define BLE_EN  0

/* LED data structures */
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

/* I2S data structures */
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
i2s_drv_config_t hi2s;
static struct i2s_config i2s_cfg_local = {0};

/* UART data structures */
const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

static int i2s_rxtx_init(void);
static int uart_init(void);
static int led_init(void);

static int i2s_start_transfer(i2s_drv_config_t *hi2s);
static int i2s_continue_transfer(i2s_drv_config_t *hi2s);

static void bt1036c_init(void);

static void data_elab(int32_t *pmem, uint32_t block_size);

K_MEM_SLAB_DEFINE(rxtx_mem_slab, BLOCK_SIZE, NUM_BLOCKS, 4);

int main(void)
{
    /* LED init */
    led_init();

    /* I2S init */
    i2s_rxtx_init();

    /* UART init */
    uart_init();

    /* Filter init */
    lowpass_filter_init(); 

#if (BLE_EN)
    /* BLE init */
    ble_init();
#endif // BLE_EN

    k_sleep(K_MSEC(500));

#if (BLE_EN)
    /* Start peripheral advertising */
    ble_start_adv();
#endif // BLE_EN

    /* Config bluetooth module */
    bt1036c_init();

    /* App is running */
    gpio_pin_set(led.port, led.pin, 1);

    /* Start the transmission */
    i2s_start_transfer(&hi2s); // I2S peripheral needs at least 2 blocks ready before the trigger start

    /* Trigger start*/
    i2s_trigger_txrx(&hi2s);

    /* Bare metal settings
     *
     *  NOTE1; these settings are not possible with
     *        Zephyr APIs.
     *
     *  NOTE2; these settings must be set after i2s_trigger_txrx() to have effect.
     */
    NRF_I2S0->CONFIG.MCKFREQ = 0x66681000; // Calculated via script octave
    NRF_I2S0->CONFIG.RATIO = 7;            // 384 (stick to octave calculations)

    while (1)
    {
        k_sleep(K_FOREVER);
        /* RX and TX continuosly */
        i2s_continue_transfer(&hi2s);
    }
    return 0;
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
    hi2s.i2s_elab = data_elab;

    i2s_cfg_local.word_size = I2S_WORD_BYTES * 8;
    i2s_cfg_local.channels = CHANNELS_NUMBER;
    i2s_cfg_local.format = I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED;
    i2s_cfg_local.frame_clk_freq = SAMPLE_FREQ;
    i2s_cfg_local.block_size = BLOCK_SIZE;
    i2s_cfg_local.timeout = I2S_RX_DELAY; // This is the max read delay before the i2s_read fails
    i2s_cfg_local.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER; // Set the microcontroller as I2S master and generator of MCK and BCK
    i2s_cfg_local.mem_slab = &rxtx_mem_slab;

    return i2s_config(&hi2s);
}

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

static void bt1036c_init(void)
{
    bt1036c_config(uart0_dev);
}

/**
 * @brief i2s_start_transfer; at least 2 blocks must be allocated before triggering the start
 *
 * @param hi2s
 * @return int
 */
static int i2s_start_transfer(i2s_drv_config_t *hi2s)
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
 * @brief i2s_continue_transfer
 *
 * @param hi2s
 * @return int
 */
static int i2s_continue_transfer(i2s_drv_config_t *hi2s)
{
    i2s_drv_txrx(hi2s);
#if (I2S_DEBUG == 1)
    int free_blocks = k_mem_slab_num_free_get(&rxtx_mem_slab);
    printk("Blocchi slab disponibili: %d\n", free_blocks);
#endif
    return 1;
}

/**
 * @brief led_init
 *
 * @return int
 */
int led_init(void)
{
    if (!gpio_is_ready_dt(&led))
    {
        return 0;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    return 1;
}

static void data_elab(int32_t *pmem, uint32_t block_size)
{
    int size = block_size / sizeof(int32_t);

    //const float max = MAX_LIMIT;
    //const float min = MIN_LIMIT;

    float32_t data_f32 = 0.0;
    q15_t data_q15;

    for (int i = 0; i < size; i++)
    {

        data_f32 = ((pmem[i])/(float32_t)2147483648);

        arm_float_to_q15(&data_f32, &data_q15, 1);

        pmem[i] = (int32_t)(lowpass_filter_exc(&data_q15) * (2147483648/32768)*10);

/*
        if ((pmem[i] <= max) && (pmem[i] >= min))
        {
            pmem[i] <<= AMP_FACTOR;
        }
        else
        {
            pmem[i] = (pmem[i] >= 0) ? INT32_MAX : INT32_MIN;
        }
*/
    }
}