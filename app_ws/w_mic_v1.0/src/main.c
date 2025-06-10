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

/* Apps */
#include "audio.h"

/* Support */
#include <math.h>

#define SIG_GENERATOR 2

/* PWM data structure */
const struct pwm_dt_spec out_pwm = PWM_DT_SPEC_GET(DT_NODELABEL(out_pwm0));

/* GPIO data structures */
static const struct gpio_dt_spec out = GPIO_DT_SPEC_GET(DT_NODELABEL(out0), gpios);

/* Static function prototypes */
static int periph_config(void);

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

	audio_process();

	k_sleep(K_FOREVER);
	while (1)
	{
	}
#endif

	return 1;
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
/*
	if (adc_config() < 0)
	{
		printk("Error: ADC device not ready\r\n");
		return -1;
	}
*/
#endif
/*
	if (i2s_config() < 0)
	{
		printk("Error: I2S device not ready\r\n");
		return -1;
	}
*/
	return 0;
}