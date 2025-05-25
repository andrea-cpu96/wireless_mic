/* System */
#include <zephyr/kernel.h>

/* Peripheral drivers */
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
/* Standard C libraries */
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    const struct device *adc_dev;
    struct adc_channel_cfg channel_cfg;
    struct adc_sequence sequence;
    int16_t *data_mv;
    int16_t *data_buff;
    struct k_poll_signal *adc_sig;
    bool async;
} adc_handler_t;

int adc_drv_config(adc_handler_t adc_handler);
int adc_drv_read(adc_handler_t adc_handler);
int16_t adc_drv_get(void);