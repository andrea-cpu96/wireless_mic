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

/* System */
#include <zephyr/kernel.h>
/* Zephyr peripheral drivers */
#ifdef CONFIG_ADC
#include <zephyr/drivers/adc.h>
#endif
/* Nordic peripheral drivers */
#ifdef CONFIG_NRFX_SAADC
#include "nrf_adc_drv.h"
#endif
/* Standard C libraries */
#include <stdint.h>
#include <stdbool.h>

typedef enum ADC_DRV_MODE
{
    ADC_DRV_SYNC,       // Blocking mode
    ADC_DRV_ASYNC,      // Non blocking mode
    ADC_DRV_ASYNC_CONT, // Non blocking and continuos mode
} adc_drv_mode_t;

typedef struct
{
    int16_t *buffer;
    uint8_t buffers_number;
    uint16_t buffer_size;
} adc_drv_adv_buffer_settings_t;

typedef struct
{
#ifdef CONFIG_NRFX_SAADC
    adc_drv_adv_timer_settings_t timer;
    adc_drv_adv_ppi_settings_t ppi;
    nrfx_saadc_adv_config_t saadc;
    saadc_event_handler_t saadc_event_handler;
#endif
    adc_drv_adv_buffer_settings_t buffer_config;
    bool adv_default;
} adc_drv_adv_settings_t;

typedef struct
{
    struct k_poll_signal *adc_sig;
    adc_drv_adv_settings_t adv_settings;
} adc_drv_options_t;

typedef struct
{
    const struct device *adc_dev; // ADC instance
#ifdef CONFIG_ADC
    struct adc_channel_cfg channel_cfg; // ADC channel configurations
    struct adc_sequence sequence;       // ADC global configurations (buffer, resolution, channel reference)
#endif
    adc_drv_mode_t mode;
    adc_drv_options_t opt;
} adc_handler_t;

int adc_drv_config(adc_handler_t *adc_handler);
int adc_drv_read(adc_handler_t *adc_handler);

#endif /* ADC_DRV */
