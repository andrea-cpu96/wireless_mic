#ifndef NRF_ADC_DRV
#define NRF_ADC_DRV

/* Nordic peripheral drivers */
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <nrfx_saadc.h>

#include "adc_drv.h"

typedef void (*saadc_event_handler_t)(nrfx_saadc_evt_t const *p_event);

typedef struct
{
#if CONFIG_NRFX_TIMER
  // const nrfx_timer_t *timer_instance;
#endif
} adc_drv_adv_timer_settings_t;

typedef struct
{
  uint8_t m_saadc_sample_ppi_channel;
  uint8_t m_saadc_start_ppi_channel;
} adc_drv_adv_ppi_settings_t;

nrf_adc_drv_continuous(adc_handler_t *adc_handler);

#endif /* NRF_ADC_DRV */
