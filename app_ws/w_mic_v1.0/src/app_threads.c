#include "app_threads.h"
#include "app_peripherals.h"
#include "app_config.h"

K_THREAD_STACK_DEFINE(i2s_stack, I2S_STACK_SIZE);
K_THREAD_STACK_DEFINE(adc_stack, ADC_STACK_SIZE);

struct k_thread i2s_tcb;
struct k_thread adc_tcb;

volatile int16_t a = 0; // Debug variable

/**
 * @brief adc_thread
 *
 * @param p1
 * @param p2
 * @param p3
 */
void adc_thread(void *p1, void *p2, void *p3)
{
    nrfx_err_t err;

    while (1)
    {
        k_sem_take(&adc_sem, K_FOREVER);
        a = *((int16_t *)(data_buffer[block_adc_idx]));

        /* Set up the next available buffer block */
        block_adc_idx = (block_adc_idx) ? 0 : 1;
        err = nrfx_saadc_buffer_set(data_buffer[block_adc_idx], DATA_BUFFER_SIZE_16);
        if (err != NRFX_SUCCESS)
        {
            printk("ADC; buffer set error\r\n");
            return;
        }
        k_sem_give(&i2s_sem);
    }
}

int16_t tx_i2s[DATA_BUFFER_SIZE_16 * 2] = {0};

/**
 * @brief i2s_thread
 *
 * @param p1
 * @param p2
 * @param p3
 */
void i2s_thread(void *p1, void *p2, void *p3)
{
    int16_t *ptx_data;
    // int first = 1;

    while (1)
    {
        k_sem_take(&i2s_sem, K_FOREVER);

        block_i2s_idx = (block_adc_idx) ? 0 : 1; // Toggle between the two blocks

        ptx_data = data_buffer[block_i2s_idx];

        for (int i = 0; i < DATA_BUFFER_SIZE_16; i++)
        {
            tx_i2s[2 * i] = ptx_data[i]; // Left channel
            // tx_i2s[2 * i + 1] = 0;       // Right channel
        }

        /* Write data over I2S queue */
        if (i2s_write(i2s_dev, tx_i2s, DATA_BUFFER_SIZE_8) < 0)
        {
            printf("Could not write TX block\n");
            return;
        }

        if (i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) < 0)
        {
            printf("Could not trigger I2S\n");
            return;
        }

        i2s_drv_drain(&hi2s);
    }
}

/**
 * @brief adc_thread_create
 *
 */
void adc_thread_create(void)
{
    k_thread_create(&adc_tcb, adc_stack, ADC_STACK_SIZE,
                    adc_thread,
                    NULL, NULL, NULL,
                    ADC_PRIORITY, 0, K_MSEC(10));
}

/**
 * @brief i2s_thread_create
 *
 */
void i2s_thread_create(void)
{
    k_thread_create(&i2s_tcb, i2s_stack, I2S_STACK_SIZE,
                    i2s_thread,
                    NULL, NULL, NULL,
                    I2S_PRIORITY, 0, K_MSEC(1000));
}
