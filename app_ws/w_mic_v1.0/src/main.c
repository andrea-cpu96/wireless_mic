/* System */
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
/* Debug support */
#include <zephyr/sys/printk.h>
/* Zephyr peripheral drivers */
#include <zephyr/drivers/gpio.h>
/* nrf peripheral drivers */
#include <nrfx_timer.h>
#include <nrfx_dppi.h>
/* Custom peripheral drivers */
#include "pwm_drv.h"
#include "adc_drv.h"

#define SIG_GENERATOR 1

/* ADC definitions */
/* Channel */
#define ADC_CHANNEL 0
#define ADC_RESOLUTION 12
#define ADC_BUFFER_SIZE 512
/* Thread */
#define ADC_STACK_SIZE 1024
#define ADC_PRIORITY 7

/* PWM definitions */
#define PWM_FREQ_KHZ 100
#define PWM_PERIOD PWM_USEC(1U * (1000 / PWM_FREQ_KHZ))

/* Thread data structures */
K_THREAD_STACK_DEFINE(adc_stack, ADC_STACK_SIZE);
struct k_thread adc_tcb;

/* ADC data structures */
const struct device *adc = DEVICE_DT_GET(DT_NODELABEL(adc));
struct k_poll_signal adc_sig;
struct k_poll_event adc_event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
														 K_POLL_MODE_NOTIFY_ONLY,
														 &adc_sig);
adc_handler_t hadc;
static int16_t adc_sample_buffer[ADC_BUFFER_SIZE];
int16_t adc_data;

/* PWM data structure */
const struct pwm_dt_spec out_pwm = PWM_DT_SPEC_GET(DT_NODELABEL(out_pwm0));

/* GPIO data structures */
static const struct gpio_dt_spec out = GPIO_DT_SPEC_GET(DT_NODELABEL(out0), gpios);

/* Static function prototypes */
static int adc_init(void);

/**
 * @brief adc_thread
 */
void adc_thread(void *p1, void *p2, void *p3)
{
	while (1)
	{
		adc_drv_read(hadc);
		k_poll(&adc_event, 1, K_FOREVER);
		k_poll_signal_reset(&adc_sig);
		adc_data = (int16_t)(1000 * (3.6 * (adc_sample_buffer[0] / 4096.0)));
		// printk("ADC; %d\r\n", adc_data);
		k_sleep(K_MSEC(1));
	}
}

/**
 * @brief main
 *
 * @return int
 */
int main(void)
{
	if (!gpio_is_ready_dt(&out))
	{
		printk("Error: PWM device is not ready\n");
		return 1;
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
		return 1;
	}
#endif

	if (adc_init() < 0)
	{
		printk("ADC device not ready\r\n");
	}
	else
	{
		printk("ADC OK\r\n");
		
#if (SIG_GENERATOR == 0)
		k_thread_create(&adc_tcb, adc_stack, ADC_STACK_SIZE,
						adc_thread,
						NULL, NULL, NULL,
						ADC_PRIORITY, 0, K_NO_WAIT);
#endif
	}

#if (SIG_GENERATOR == 1)
	pwm_drv_sin_gen(50);

	while (1)
	{
		pwm_drv_sig_out(&out_pwm);
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
	k_poll_signal_init(&adc_sig);

	hadc.adc_dev = adc;
	hadc.adc_sig = &adc_sig;
	hadc.data_mv = &adc_data;
	hadc.data_buff = adc_sample_buffer; // The ADC internal easyDMA automatically transfers data to the RAM region identified by this buffer
	hadc.async = 1;

	hadc.channel_cfg.gain = ADC_GAIN_1_6;
	hadc.channel_cfg.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 3);
	hadc.channel_cfg.reference = ADC_REF_INTERNAL;
	hadc.channel_cfg.channel_id = ADC_CHANNEL;
	hadc.channel_cfg.input_positive = NRF_SAADC_AIN0;

	hadc.sequence.channels = BIT(ADC_CHANNEL);
	hadc.sequence.buffer = adc_sample_buffer;
	hadc.sequence.buffer_size = sizeof(adc_sample_buffer);
	hadc.sequence.resolution = ADC_RESOLUTION;

	return adc_drv_config(hadc);
}