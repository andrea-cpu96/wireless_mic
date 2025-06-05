/**
 * @file adc_drv.c
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides an interface to the ADC peripheral.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

/**
 * TODO
 * Create unit tests to verify the correct function of both
 * the default modes and the mode with personal settings.
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
	nrf_adc_drv_continuous(&adc_handler->nrf_saadc_config);
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