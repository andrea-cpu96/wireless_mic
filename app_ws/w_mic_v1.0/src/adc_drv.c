/* System */
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include "adc_drv.h"

int adc_drv_config(adc_handler_t adc_handler)
{
	if (!device_is_ready(adc_handler.adc_dev))
	{
		return -1;
	}

	adc_channel_setup(adc_handler.adc_dev, &adc_handler.channel_cfg);

	return 0;
}

int adc_drv_read(adc_handler_t adc_handler)
{
	if (adc_handler.async == 1)
	{
		adc_read_async(adc_handler.adc_dev, &adc_handler.sequence, adc_handler.adc_sig);
		return 0;
	}

	if (adc_read(adc_handler.adc_dev, &adc_handler.sequence) != 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}