/**
 * @file pwm_drv.h
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides an interface to the PWM peripheral.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

#ifndef PWM_DRV
#define PWM_DRV

/* System */
#include <zephyr/kernel.h>
/* Peripheral drivers */
#include <zephyr/drivers/pwm.h>
/* Standard C libraries */
#include <stdint.h>
#include <stdbool.h>

/* PWM definitions */
#define PWM_FREQ_KHZ 100
#define PWM_PERIOD PWM_USEC(1U * (1000 / PWM_FREQ_KHZ))

#define SIG_NUM_OF_SAMPLES 20000

#define DELAY_20US_NCYCLE 71 // Corrisponds to about 20us at 64MHz

extern volatile uint16_t idx_step;
extern volatile uint16_t idx;
extern volatile double pwm_samples[SIG_NUM_OF_SAMPLES];

int pwm_drv_config(const struct pwm_dt_spec *pwm);
int pwm_drv_sin_freq_set(uint16_t f);
void pwm_drv_sin_gen(uint16_t f);

static inline void pwm_drv_sig_out(const struct pwm_dt_spec *pwm)
{
    pwm_set_dt(pwm, PWM_PERIOD, pwm_samples[idx]);
    idx = (idx + idx_step) % SIG_NUM_OF_SAMPLES;

    for (volatile uint32_t i = 0; i < (DELAY_20US_NCYCLE); i++)
    {
        __asm__ volatile("nop");
    }
}

#endif /* PWM_DRV */