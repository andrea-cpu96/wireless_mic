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
/* Zephyr peripheral drivers */
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

#include "adc_drv.h"

#define SAADC_SAMPLE_INTERVAL_US 5
#define TIMER_INSTANCE_NUMBER 2

#if CONFIG_NRFX_TIMER
/** TODO
 * Hardwired
 */
const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER); // Timer 2 instance
#endif

#ifndef CONFIG_ADC
static int adc_drv_adv_config(adc_handler_t *adc_handler);
#endif
#ifdef CONFIG_NRFX_TIMER
static int adc_drv_adv_timer_config(void);
#endif
#ifdef CONFIG_NRFX_GPPI
static int adc_drv_adv_ppi_configure(void);
#endif

adc_drv_adv_buffer_settings_t *saadc_buffer;
static uint32_t saadc_current_buffer = 0;

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
	nrfx_err_t err;
	switch (p_event->type)
	{
	case NRFX_SAADC_EVT_READY: // First buffer initialized event

		/* Enable timer (this also enable the sampling triggered by the PPI) */
		nrfx_timer_enable(&timer_instance);
		break;

	case NRFX_SAADC_EVT_BUF_REQ: // A buffer has been required by the ADC

		/* Set up the next available buffer. Alternate between buffer 0 and 1 */
		err = nrfx_saadc_buffer_set(&saadc_buffer->buffer[((saadc_current_buffer)*saadc_buffer->buffer_size)], saadc_buffer->buffer_size);
		saadc_current_buffer = (saadc_current_buffer + 1) % saadc_buffer->buffers_number;
		if (err != NRFX_SUCCESS)
		{
			printk("ADC; buffer set error\r\n");
			return;
		}
		break;

	case NRFX_SAADC_EVT_DONE: // A buffer has been filled
		break;

	default:
		printk("ADC; event error\r\n");
		break;
	}
}

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
	if (adc_handler->mode == ADC_DRV_ASYNC_CONT)
	{
		saadc_buffer = &adc_handler->opt.adv_settings.buffer_config;
		adc_handler->opt.adv_settings.saadc_event_handler = saadc_event_handler;
		adc_drv_adv_timer_config();
		adc_drv_adv_config(adc_handler);
		adc_drv_adv_ppi_configure();
	}

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
	err = nrfx_timer_init(&timer_instance, &timer_config, NULL);		   // The third parameter is the reference to the event handler (NULL if none)
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
									  nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE));				// Set SAMPLE ADC task as action

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