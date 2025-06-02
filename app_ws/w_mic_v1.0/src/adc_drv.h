/* System */
#include <zephyr/kernel.h>

/* Peripheral drivers */
#ifdef CONFIG_ADC
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#endif
#ifdef CONFIG_NRFX_SAADC
#include <nrfx_saadc.h>
#endif
/* Standard C libraries */
#include <stdint.h>
#include <stdbool.h>

#ifndef CONFIG_ADC
typedef void (*saadc_event_handler_t)(nrfx_saadc_evt_t const *p_event);
#endif

typedef enum ADC_DRV_MODE
{
    ADC_DRV_SYNC,       // Blocking mode
    ADC_DRV_ASYNC,      // Non blocking mode
    ADC_DRV_ASYNC_CONT, // Non blocking and continuos mode
} adc_drv_mode_t;

typedef struct
{
#if CONFIG_NRFX_TIMER2
    // const nrfx_timer_t *timer_instance;
#endif
} adc_drv_adv_timer_settings_t;

typedef struct
{
    uint8_t m_saadc_sample_ppi_channel;
    uint8_t m_saadc_start_ppi_channel;
} adc_drv_adv_ppi_settings_t;

typedef struct
{
    int16_t *buffer;
    uint8_t buffers_number;
    uint16_t buffer_size;
} adc_drv_adv_buffer_settings_t;

typedef struct
{
    adc_drv_adv_timer_settings_t timer;
    adc_drv_adv_ppi_settings_t ppi;
#ifndef CONFIG_ADC
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