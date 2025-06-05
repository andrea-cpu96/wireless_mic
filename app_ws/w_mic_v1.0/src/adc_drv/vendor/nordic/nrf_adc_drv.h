#ifndef NRF_ADC_DRV
#define NRF_ADC_DRV

/* Nordic peripheral drivers */
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <nrfx_saadc.h>

#if CONFIG_NRFX_TIMER
#include <nrfx_timer.h>
#endif
#ifdef CONFIG_NRFX_GPPI
#include <helpers/nrfx_gppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif
#endif

typedef void (*saadc_event_handler_t)(nrfx_saadc_evt_t const *p_event);

typedef struct
{
#if CONFIG_NRFX_TIMER
  nrfx_timer_t *timer_instance;
  nrfx_timer_config_t *timer_config;
  uint32_t saadc_sampling_time;
#endif
} adc_drv_adv_timer_setting_t;

typedef struct
{
  int16_t *buffer;
  uint8_t buffers_number;
  uint16_t buffer_size;
} adc_drv_adv_buffer_setting_t;

typedef struct
{
  adc_drv_adv_timer_setting_t timer;
  nrfx_saadc_adv_config_t saadc;
  nrfx_saadc_channel_t saadc_ch;
  saadc_event_handler_t saadc_event_handler;
  adc_drv_adv_buffer_setting_t buffer_config;
  bool adv_default;
} nrf_saadc_setting_t;

void nrf_adc_drv_continuous(nrf_saadc_setting_t *adc_handler);

#endif /* NRF_ADC_DRV */
