/**
 * @file adc_drv.c
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides an interface to the ADC peripheral.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

/* System */
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include "adc_drv.h"

int adc_drv_config(adc_handler_t *adc_handler)
{
#ifdef CONFIG_ADC
	if (!device_is_ready(adc_handler->adc_dev))
	{
		return -1;
	}

	adc_channel_setup(adc_handler->adc_dev, &adc_handler->channel_cfg);

	return 0;
#elif defined CONFIG_NRFX_SAADC
	nrf_adc_drv_continuous(adc_handler_t * adc_handler);
	return 0;
#else

	return -1;
#endif

	return 0;
}

int adc_drv_read(adc_handler_t *adc_handler)
{
	switch (adc_handler->mode)
	{
	case ADC_DRV_SYNC:
#ifdef CONFIG_ADC
		if (adc_read(adc_handler->adc_dev, &adc_handler->sequence) != 0)
		{
			return -1;
		}
#else
		return -1;
#endif
		break;
	case ADC_DRV_ASYNC:
#ifdef CONFIG_ADC_ASYNC
		adc_read_async(adc_handler->adc_dev, &adc_handler->sequence, adc_handler->opt.adc_sig);
#else
		return -1;
#endif
		break;
	case ADC_DRV_ASYNC_CONT:
		return -1;
		break;
	default:
		return -1;
		break;
	}

	return 0;
}

#ifndef CONFIG_ADC
static int adc_drv_adv_config(adc_handler_t *adc_handler)
{
	nrfx_err_t err;

	nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);
	err = nrfx_saadc_channels_config(&channel, 1);
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; channel config error\r\n");
		return -1;
	}

	if (adc_handler->opt.adv_settings.adv_default)
	{
		nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
		err = nrfx_saadc_advanced_mode_set(BIT(0),
																			 12, /** TODO should be configurable */
																			 &saadc_adv_config,
																			 adc_handler->opt.adv_settings.saadc_event_handler); // If NULL it will work in blocking mode
	}
	else
	{
		err = nrfx_saadc_advanced_mode_set(BIT(0),
																			 12, /** TODO should be configurable */
																			 &adc_handler->opt.adv_settings.saadc,
																			 adc_handler->opt.adv_settings.saadc_event_handler); // If NULL it will work in blocking mode
	}

	if (err != NRFX_SUCCESS)
	{
		printk("ADC; advanced config error\r\n");
		return -1;
	}

	for (int i = 0; i < adc_handler->opt.adv_settings.buffer_config.buffers_number; i++)
	{
		err = nrfx_saadc_buffer_set(&adc_handler->opt.adv_settings.buffer_config.buffer[i * adc_handler->opt.adv_settings.buffer_config.buffer_size], adc_handler->opt.adv_settings.buffer_config.buffer_size);
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
#endif

#ifdef CONFIG_NRFX_TIMER
static int adc_drv_adv_timer_config(void)
{
	/** TODO
	 * Hardwired
	 */
	nrfx_err_t err;

	nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG(1000000); // Timer frequency
	err = nrfx_timer_init(&timer_instance, &timer_config, NULL);					 // The third parameter is the reference to the event handler (NULL if none)
	if (err != NRFX_SUCCESS)
	{
		printk("TIMER; init error\r\n");
		return -1;
	}

	uint32_t timer_ticks = nrfx_timer_us_to_ticks(&timer_instance, SAADC_SAMPLE_INTERVAL_US);
	nrfx_timer_extended_compare(&timer_instance, NRF_TIMER_CC_CHANNEL0, timer_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false); // Compare the timer value with the number of ticks passed and creates an event, also specify an action (reset the timer)

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
																		nrfx_timer_compare_event_address_get(&timer_instance, NRF_TIMER_CC_CHANNEL0), // Set COMPARE0 timer event as trigger
																		nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));								// Set SAMPLE ADC task as action

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
