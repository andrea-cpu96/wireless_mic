#ifndef APP_CONFIG
#define APP_CONFIG

/* System */
#include <zephyr/kernel.h>

#define NUM_OF_BUFFERS 2

#define DATA_BUFFER_SIZE_16 (10000)
#define DATA_BUFFER_SIZE_8 (DATA_BUFFER_SIZE_16 * 2)

#define NUM_BLOCKS NUM_OF_BUFFERS // Each block is a buffer (2 buffers; 1 to read and 1 to write simultaneosly + 2 backup buffers)
#define BLOCK_SIZE (DATA_BUFFER_SIZE_8) // Buffer size multiplied by the number of bytes per data

extern int16_t *data_buffer[NUM_OF_BUFFERS];
extern volatile int block_adc_idx; // Index of the current block being processed by the ADC
extern volatile int block_i2s_idx; // Index of the current block being processed by the I2S

extern struct k_sem adc_sem;
extern struct k_sem i2s_sem;

extern struct k_mem_slab tx_0_mem_slab;

extern struct k_poll_signal adc_sig;
extern struct k_poll_event adc_event;

#endif /* APP_CONFIG */
