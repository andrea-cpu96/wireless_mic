/* System */
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include <math.h>
#include "pwm_drv.h"

#define PI 3.14159

#define SIN_FREQ_FACTOR 4 // Corresponds to 10Hz (min freq)

volatile uint16_t idx_step = 0;
volatile uint16_t idx = 0;
volatile double pwm_samples[SIG_NUM_OF_SAMPLES] = {0};

int pwm_drv_config(const struct pwm_dt_spec *pwm)
{
    pwm_drv_sin_freq_set(SIN_FREQ_FACTOR);

    if (!pwm_is_ready_dt(pwm))
    {
        return -1;
    }
    pwm_set_dt(pwm, PWM_PERIOD, PWM_PERIOD / 2U);

    return 0;
}

void pwm_drv_sin_gen(uint16_t f)
{
    double sin_sig;
    double dt = (1.0 / SIG_NUM_OF_SAMPLES);
    double t = 0;

    pwm_drv_sin_freq_set(f);

    for (int i = 0; i < SIG_NUM_OF_SAMPLES; i++)
    {
        sin_sig = (sin(2 * PI * 1 * t) + 1U); // 1Hz sin reference (in that way each SIG_NUM_OF_SAMPLES is equivalent to the sample frequency)
        pwm_samples[i] = (PWM_PERIOD - (PWM_PERIOD * (sin_sig / 2.0)));
        t += dt;
    }
}

int pwm_drv_sin_freq_set(uint16_t f)
{
    if (f < 10)
    {
        idx_step = (SIN_FREQ_FACTOR);
        return -1;
    }

    idx_step = (SIN_FREQ_FACTOR * f / 10);
    return 0;
}