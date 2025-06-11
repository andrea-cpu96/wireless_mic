/* System */
#include <zephyr/kernel.h>

#ifndef APP_THREADS
#define APP_THREADS

/* I2S thread */
#define I2S_STACK_SIZE 1024
#define I2S_PRIORITY 6

/* ADC thread */
#define ADC_STACK_SIZE 1024
#define ADC_PRIORITY 7

void adc_thread_create(void);
void i2s_thread_create(void);

#endif /* APP_THREADS */
