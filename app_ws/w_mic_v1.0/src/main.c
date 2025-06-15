#define SYNTHETIZER 0
#define SIG_GENERATOR 2

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
#if (SYNTHETIZER == 1)
#include "i2s_sample.h"
#else
#include "audio.h"
#endif

/* Support */
#include <math.h>

/* PWM data structure */
const struct pwm_dt_spec out_pwm = PWM_DT_SPEC_GET(DT_NODELABEL(out_pwm0));

/* GPIO data structures */
static const struct gpio_dt_spec out = GPIO_DT_SPEC_GET(DT_NODELABEL(out0), gpios);

/* Static function prototypes */
static int periph_config(void);

/* I2S data structures */
#if (SYNTHETIZER == 1)
const struct device *my_i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));
#endif

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
	pwm_drv_sin_gen(1000);

	while (1)
	{
		pwm_drv_sig_out(&out_pwm);
	}
#else

#if (SYNTHETIZER == 1)
	i2s_sample_config(my_i2s_dev);
	i2s_sample(my_i2s_dev);
#else
	audio_process();
#endif

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
#endif
	return 0;
}