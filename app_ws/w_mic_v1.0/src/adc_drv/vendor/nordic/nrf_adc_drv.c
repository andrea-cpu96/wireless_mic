/* Debug support */
#include <zephyr/sys/printk.h>

#include "nrf_adc_drv.h"

#define SAADC_SAMPLE_INTERVAL_US_DEFAULT 5
#define TIMER_INSTANCE_NUMBER_DEFAULT 2

#if CONFIG_NRFX_TIMER
/* Timer 2 instance */
nrfx_timer_t timer_instance_default = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER_DEFAULT);
nrfx_timer_t *timer_instance;
/* Timer frequency */
nrfx_timer_config_t timer_config_default = NRFX_TIMER_DEFAULT_CONFIG(1000000);
nrfx_timer_config_t *timer_config;
#endif

/**
 * TODO
 * Kconfig conditioned by the presence of TIMER and PPI
 */

#ifdef CONFIG_NRFX_SAADC
static int adc_drv_adv_config(nrf_saadc_setting_t *adc_handler);
#endif
#ifdef CONFIG_NRFX_TIMER
static int adc_drv_adv_timer_config(adc_drv_adv_timer_setting_t timer);
#endif
#ifdef CONFIG_NRFX_GPPI
static int adc_drv_adv_ppi_configure(void);
#endif

void nrf_adc_drv_continuous(nrf_saadc_setting_t *adc_handler)
{
#ifdef CONFIG_NRFX_TIMER
	adc_drv_adv_timer_config(adc_handler->timer);
#endif
	adc_drv_adv_config(adc_handler);
	adc_drv_adv_ppi_configure();
}

static int adc_drv_adv_config(nrf_saadc_setting_t *adc_handler)
{
	nrfx_err_t err;

	nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(adc_handler->saadc_ch.pin_p, 0);
	err = nrfx_saadc_channels_config(&channel, 1); /** TODO configure number of channels */
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; channel config error\r\n");
		return -1;
	}

	if (adc_handler->adv_default)
	{
		nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
		err = nrfx_saadc_advanced_mode_set(BIT(0),
										   12, /** TODO should be configurable */
										   &saadc_adv_config,
										   adc_handler->saadc_event_handler); // If NULL it will work in blocking mode
	}
	else
	{
		err = nrfx_saadc_advanced_mode_set(BIT(0),
										   12, /** TODO should be configurable */
										   &adc_handler->saadc,
										   adc_handler->saadc_event_handler); // If NULL it will work in blocking mode
	}

	if (err != NRFX_SUCCESS)
	{
		printk("ADC; advanced config error\r\n");
		return -1;
	}

	for (int i = 0; i < adc_handler->buffer_config.buffers_number; i++)
	{
		err = nrfx_saadc_buffer_set(&adc_handler->buffer_config.buffer[i * adc_handler->buffer_config.buffer_size], adc_handler->buffer_config.buffer_size);
		if (err != NRFX_SUCCESS)
		{
			printk("ADC; buffer set error\r\n");
			return -1;
		}
	}

	/* Set a buffer for sampling triggered later through PPI */
	err = nrfx_saadc_mode_trigger();
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; ADC trigger error\r\n");
		return -1;
	}

	return 0;
}

#ifdef CONFIG_NRFX_TIMER
static int adc_drv_adv_timer_config(adc_drv_adv_timer_setting_t timer)
{
	nrfx_err_t err;

	if (timer.timer_instance == NULL)
	{
		timer_instance = &timer_instance_default; // Timer 2 instance
		timer_config = &timer_config_default;	  // Timer frequency
		timer.timer_instance = timer_instance;
	}
	else
	{
		timer_instance = timer.timer_instance; // Timer instance
		timer_config = timer.timer_config;	   // Timer frequency
	}

	err = nrfx_timer_init(timer_instance, timer_config, NULL); // The third parameter is the reference to the event handler (NULL if none)
	if (err != NRFX_SUCCESS)
	{
		printk("TIMER; init error\r\n");
		return -1;
	}

	/**
	 * TODO
	 * channel 0 is hardwired
	 */
	uint32_t timer_ticks = nrfx_timer_us_to_ticks(timer_instance, timer.saadc_sampling_time);
	nrfx_timer_extended_compare(timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false); // Compare the timer value with the number of ticks passed and creates an event, also specify an action (reset the timer)

	return 0;
}
#endif

#ifdef CONFIG_NRFX_GPPI
static int adc_drv_adv_ppi_configure(void)
{
	nrfx_err_t err;

	/* Declare variables used to hold the (D)PPI channel number */
	uint8_t m_saadc_sample_ppi_channel;
	uint8_t m_saadc_start_ppi_channel;

	/* Trigger ADC SAMPLE TASK */
	err = nrfx_gppi_channel_alloc(&m_saadc_sample_ppi_channel);
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; task trigger error\r\n");
		return -1;
	}

	/* Trigger ADC START task */
	err = nrfx_gppi_channel_alloc(&m_saadc_start_ppi_channel);
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; task trigger error\r\n");
		return -1;
	}

	/* Trigger task sample from timer */
	nrfx_gppi_channel_endpoints_setup(m_saadc_sample_ppi_channel,
									  nrfx_timer_compare_event_address_get(timer_instance, NRF_TIMER_CC_CHANNEL0), // Set COMPARE0 timer event as trigger
									  nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));			   // Set SAMPLE ADC task as action

	/* Trigger task start from end event */
	nrfx_gppi_channel_endpoints_setup(m_saadc_start_ppi_channel,
									  nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END),	// Set END ADC event as trigger (the current buffer is full, BUFF_REQ event is triggerd)
									  nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START)); // Set START ADC task as action (start sampling again)

	/* Enable both (D)PPI channels */
	nrfx_gppi_channels_enable(BIT(m_saadc_sample_ppi_channel));
	nrfx_gppi_channels_enable(BIT(m_saadc_start_ppi_channel));

	return 0;
}
#endif
