/* System */
#include <zephyr/devicetree.h>

#include "app_peripherals.h"
#include "app_config.h"

/* I2S data structures */
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
i2s_drv_config_t hi2s;
static struct i2s_config i2s_cfg_local = {0};

/* ADC data structures */
const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));
adc_handler_t hadc;

int16_t *data_buffer[NUM_OF_BUFFERS];
int block_adc_idx = 0;
int block_i2s_idx = 1;

/**
 * @brief saadc_event_handler
 *
 * @param p_event
 */
static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
    switch (p_event->type)
    {
    case NRFX_SAADC_EVT_READY: // First buffer initialized event

        /* Enable timer (this also enable the sampling triggered by the PPI) */
        nrfx_timer_enable(hadc.nrf_saadc_config.timer.timer_instance);
        break;

    case NRFX_SAADC_EVT_BUF_REQ: // A buffer has been required by the ADC
        k_sem_give(&adc_sem);
        break;

    case NRFX_SAADC_EVT_DONE: // A buffer has been filled
        break;

    default:
        printk("ADC; event error\r\n");
        break;
    }
}

/**
 * @brief adc_config
 *
 * @return int
 */
int adc_config(void)
{
    volatile void *adc_block;
    int err;
    /* Connect ADC interrupt to nrfx interrupt handler */
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),           // Extract irq number
                DT_IRQ(DT_NODELABEL(adc), priority),  // Extract irq priority
                nrfx_isr, nrfx_saadc_irq_handler, 0); // Connect the interrupt to the nrf interrupt handler

    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    if (err != NRFX_SUCCESS)
    {
        printk("ADC; init error\r\n");
        return -1;
    }

    for (int i = 0; i < 2; i++)
    {
        /* One block allocated for each cycle */
        if (k_mem_slab_alloc(&tx_0_mem_slab, (void **)&adc_block, K_FOREVER) < 0)
        {
            printf("Failed to allocate TX block\n");
            return -1;
        }
        data_buffer[i] = (uint16_t *)adc_block;
    }

    hadc.mode = ADC_DRV_ASYNC_CONT;
    hadc.nrf_saadc_config.adv_default = 1;
    hadc.nrf_saadc_config.buffer_config.buffer = data_buffer; // The ADC internal easyDMA automatically transfers data to the RAM region identified by this buffer
    hadc.nrf_saadc_config.buffer_config.buffers_number = NUM_OF_BUFFERS;
    hadc.nrf_saadc_config.buffer_config.buffer_size = DATA_BUFFER_SIZE_16;
    hadc.nrf_saadc_config.saadc_event_handler = saadc_event_handler;
    hadc.nrf_saadc_config.saadc_ch.channel_config.gain = ADC_GAIN_1_6;
    hadc.nrf_saadc_config.saadc_ch.pin_p = NRF_SAADC_INPUT_AIN2;

    hadc.nrf_saadc_config.timer.timer_instance = NULL;      // Default
    hadc.nrf_saadc_config.timer.timer_config = NULL;        // Default
    hadc.nrf_saadc_config.timer.saadc_sampling_time = 22; // 22us -> 44100 kHz

    return adc_drv_config(&hadc); 
}

/**
 * @brief i2s_config
 *
 * @return int
 */
int i2s_config(void)
{
    /* Check device is ready */
    if (!device_is_ready(i2s_dev))
    {
        printf("I2S device not ready\n");
        return -1;
    }

    /* Configure I2S */
    hi2s.dev_i2s = i2s_dev;
    hi2s.i2s_cfg_dir = I2S_DIR_TX;
    i2s_cfg_local.word_size = 16;
    i2s_cfg_local.channels = CHANNELS_NUMBER;
    i2s_cfg_local.format = I2S_FMT_DATA_FORMAT_I2S;
    i2s_cfg_local.frame_clk_freq = SAMPLE_FREQ;
    i2s_cfg_local.block_size = DATA_BUFFER_SIZE_8;
    i2s_cfg_local.timeout = 2000;
    i2s_cfg_local.options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER;
    i2s_cfg_local.mem_slab = &tx_0_mem_slab;

    hi2s.i2s_cfg = &i2s_cfg_local;

    return i2s_drv_config(&hi2s);
}
