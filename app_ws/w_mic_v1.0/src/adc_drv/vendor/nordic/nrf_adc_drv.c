/* Nordic peripheral drivers */
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

#define SAADC_SAMPLE_INTERVAL_US 5
#define TIMER_INSTANCE_NUMBER 2

#if CONFIG_NRFX_TIMER
/** TODO
 * Hardwired
 */
const nrfx_timer_t timer_instance = NRFX_TIMER_INSTANCE(TIMER_INSTANCE_NUMBER); // Timer 2 instance
#endif

#ifdef CONFIG_NRFX_SAADC
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

nrf_adc_drv_continuous(adc_handler_t *adc_handler)
{
	if (adc_handler->mode == ADC_DRV_ASYNC_CONT)
	{
		saadc_buffer = &adc_handler->opt.adv_settings.buffer_config;
		adc_handler->opt.adv_settings.saadc_event_handler = saadc_event_handler;
		//adc_drv_adv_timer_config();
		adc_drv_adv_config(adc_handler);
		//adc_drv_adv_ppi_configure();
	}
}
