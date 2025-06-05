/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
/* Debug support */
#include <zephyr/sys/printk.h>
/* Zephyr peripheral drivers */
#include <zephyr/drivers/gpio.h>
/* Custom peripheral drivers */
#include "pwm_drv.h"
#include "adc_drv.h"
#include "audio.h"

#define SIG_GENERATOR 2

/* ADC definitions */
/* Channel */
#define ADC_CHANNEL 0
#define ADC_RESOLUTION 12
#define ADC_BUFFER_SIZE 512

/* I2S data structures */
const struct device *const i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));

/* ADC data structures */
const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));
adc_handler_t hadc;
int16_t adc_buffer[2][10];
adc_drv_adv_buffer_setting_t *saadc_buffer = &hadc.nrf_saadc_config.buffer_config;

/* PWM data structure */
const struct pwm_dt_spec out_pwm = PWM_DT_SPEC_GET(DT_NODELABEL(out_pwm0));

/* GPIO data structures */
static const struct gpio_dt_spec out = GPIO_DT_SPEC_GET(DT_NODELABEL(out0), gpios);

/* Static function prototypes */
static int adc_init(void);
static int periph_config(void);

static void saadc_event_handler(nrfx_saadc_evt_t const *p_event)
{
	static uint32_t saadc_current_buffer = 0;
	nrfx_err_t err;
	switch (p_event->type)
	{
	case NRFX_SAADC_EVT_READY: // First buffer initialized event

		/* Enable timer (this also enable the sampling triggered by the PPI) */
		nrfx_timer_enable(hadc.nrf_saadc_config.timer.timer_instance);
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

/**
 * @brief main
 *
 * @return int
 */
int main(void)
{
	if (periph_config() < 0)
	{
		printk("Error\n");
		return 1;
	}

#if (SIG_GENERATOR == 1)
	pwm_drv_sin_gen(500);

	while (1)
	{
		pwm_drv_sig_out(&out_pwm);
	}
#else

	// i2s_config(i2s_dev);
	// i2s_sample(i2s_dev);

	k_sleep(K_FOREVER);
	while (1)
	{
	}
#endif

	return 1;
}

/**
 * @brief adc_init
 *
 * @return int
 */
static int adc_init(void)
{
	int err;
	/* Connect ADC interrupt to nrfx interrupt handler */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),			  // Extract irq number
				DT_IRQ(DT_NODELABEL(adc), priority),  // Extract irq priority
				nrfx_isr, nrfx_saadc_irq_handler, 0); // Connect the interrupt to the nrf interrupt handler

	err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
	if (err != NRFX_SUCCESS)
	{
		printk("ADC; init error\r\n");
		return -1;
	}

	hadc.mode = ADC_DRV_ASYNC_CONT;
	hadc.nrf_saadc_config.adv_default = 1;
	hadc.nrf_saadc_config.buffer_config.buffer = &adc_buffer[0][0]; // The ADC internal easyDMA automatically transfers data to the RAM region identified by this buffer
	hadc.nrf_saadc_config.buffer_config.buffers_number = 2;
	hadc.nrf_saadc_config.buffer_config.buffer_size = 10;
	hadc.nrf_saadc_config.saadc_event_handler = saadc_event_handler;
	hadc.nrf_saadc_config.saadc_ch.channel_config.gain = ADC_GAIN_1_6;
	hadc.nrf_saadc_config.saadc_ch.pin_p = NRF_SAADC_INPUT_AIN0;

	hadc.nrf_saadc_config.timer.timer_instance = NULL;	  // Default
	hadc.nrf_saadc_config.timer.timer_config = NULL;	  // Default
	hadc.nrf_saadc_config.timer.saadc_sampling_time = 10; // 10us

	return adc_drv_config(&hadc);
}

static int periph_config(void)
{
	if (!gpio_is_ready_dt(&out))
	{
		printk("Error: PWM device is not ready\n");
		return -1;
	}

	if (gpio_pin_configure_dt(&out, GPIO_OUTPUT_ACTIVE) != 0)
	{
		printk("Errore configurazione pin\n");
		return -1;
	}

#if (SIG_GENERATOR == 1)
	if (pwm_drv_config(&out_pwm) < 0)
	{
		printk("Error: PWM device is not ready\n");
		return -1;
	}
#else
	if (adc_init() < 0)
	{
		printk("Error: ADC device not ready\r\n");
		return -1;
	}
#endif

	return 0;
}