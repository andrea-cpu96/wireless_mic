#include "app_threads.h"
#include "app_peripherals.h"
#include "app_config.h"

K_THREAD_STACK_DEFINE(i2s_stack, I2S_STACK_SIZE);
K_THREAD_STACK_DEFINE(adc_stack, ADC_STACK_SIZE);

struct k_thread i2s_tcb;
struct k_thread adc_tcb;

/**
 * @brief adc_thread
 *
 * @param p1
 * @param p2
 * @param p3
 */
void adc_thread(void *p1, void *p2, void *p3)
{
    while (1)
    {
        k_sem_take(&adc_sem, K_FOREVER);

        /* Set up the next available buffer block */
        block_adc_idx = (block_adc_idx) ? 0 : 1;

        nrfx_timer_disable(hadc.nrf_saadc_config.timer.timer_instance);
        while (block_adc_idx == block_i2s_idx)
            ;
        nrfx_timer_enable(hadc.nrf_saadc_config.timer.timer_instance);

        if (nrfx_saadc_buffer_set(data_buffer[block_adc_idx], DATA_BUFFER_SIZE_16) != NRFX_SUCCESS)
        {
            printk("ADC; buffer set error\r\n");
            return;
        }
        k_sem_give(&i2s_sem);
    }
}

/**
 * @brief i2s_thread
 *
 * @param p1
 * @param p2
 * @param p3
 */
void i2s_thread(void *p1, void *p2, void *p3)
{
    static int first = 1;

    while (1)
    {
        k_sem_take(&i2s_sem, K_FOREVER);

        block_i2s_idx = (block_adc_idx) ? 0 : 1; // Toggle between the two blocks

        
        for (int i = 0; i < DATA_BUFFER_SIZE_16; i++)
            data_buffer[block_i2s_idx][i] = (data_buffer[block_i2s_idx][i] << 3);

        /* Write data over I2S queue */
        if (i2s_write(i2s_dev, data_buffer[block_i2s_idx], DATA_BUFFER_SIZE_8) < 0)
        {
            printf("Could not write TX block\n");
            return;
        }

        if (first)
        {
            first = 0;
            if (i2s_trigger(i2s_dev, I2S_DIR_TX, I2S_TRIGGER_START) < 0)
            {
                printf("Could not trigger I2S\n");
                return;
            }
        }
        k_sleep(K_MSEC(200));
        block_i2s_idx = 0xFF;
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
                    ADC_PRIORITY, 0, K_MSEC(100));
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
                    I2S_PRIORITY, 0, K_MSEC(10));
}
