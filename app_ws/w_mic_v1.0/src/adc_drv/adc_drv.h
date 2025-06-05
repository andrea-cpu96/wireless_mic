/**
 * @file adc_drv.h
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides an interface to the ADC peripheral.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

#ifndef ADC_DRV
#define ADC_DRV

/* Zephyr peripheral drivers */
#include <zephyr/drivers/adc.h>
/* Nordic peripheral drivers */
#ifdef CONFIG_NRFX_SAADC
#include "nrf_adc_drv.h"
#endif

typedef enum ADC_DRV_MODE
{
    ADC_DRV_SYNC,       // Blocking mode
    ADC_DRV_ASYNC,      // Non blocking mode
    ADC_DRV_ASYNC_CONT, // Non blocking and continuos mode
} adc_drv_mode_t;

typedef struct
{
    struct k_poll_signal *adc_sig;
} adc_drv_options_t;

typedef struct
{
    const struct device *adc_dev; // ADC instance
#ifdef CONFIG_ADC
    struct adc_channel_cfg channel_cfg; // ADC channel configurations
    struct adc_sequence sequence;       // ADC global configurations (buffer, resolution, channel reference)
#endif
#ifdef CONFIG_NRFX_SAADC
    nrf_saadc_setting_t nrf_saadc_config;
#endif
    adc_drv_mode_t mode;
    adc_drv_options_t opt;
} adc_handler_t;

int adc_drv_config(adc_handler_t *adc_handler);
int adc_drv_read(adc_handler_t *adc_handler);

#endif /* ADC_DRV */
