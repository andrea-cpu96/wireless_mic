#include "app_peripherals.h"
#include "app_threads.h"

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
    static int first = 1;     // Indicates the first iteration
    volatile void *adc_block; // Slab memory block
    nrfx_err_t err;

    while (1)
    {
        k_sem_take(&adc_sem, K_FOREVER);
        a = *((int16_t *)(adc_buffer[idx]));

        /* Free all buffer the first time  */
        if (first)
        {
            first = 0;
            k_mem_slab_free(&tx_0_mem_slab, adc_buffer[0]);
            k_mem_slab_free(&tx_0_mem_slab, adc_buffer[1]);
        }

        /* Allocate a memory block from the slab */
        if (k_mem_slab_alloc(&tx_0_mem_slab, (void **)&adc_block, K_FOREVER) < 0)
        {
            printf("Failed to allocate ADC block\n");
            return;
        }
        // Free previous buffer block
        k_mem_slab_free(&tx_0_mem_slab, adc_buffer[idx]);

        /* Set up the next available buffer block */
        idx = (idx) ? 0 : 1;
        adc_buffer[idx] = (int16_t *)adc_block;
        err = nrfx_saadc_buffer_set(adc_buffer[idx], DATA_BUFFER_SIZE_16);
        if (err != NRFX_SUCCESS)
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
    void *i2s_block[DATA_BUFFER_SIZE_8]; // Slab memory block
    static int first = 1;

    while (1)
    {
        k_sem_take(&i2s_sem, K_FOREVER);

        memcpy(i2s_block, adc_buffer[(idx) ? 0 : 1], DATA_BUFFER_SIZE_8);

        /* Write data over I2S queue */

        if (i2s_write(i2s_dev, (int16_t *)i2s_block, DATA_BUFFER_SIZE_8) < 0)
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

        // i2s_drv_drain(&hi2s);
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