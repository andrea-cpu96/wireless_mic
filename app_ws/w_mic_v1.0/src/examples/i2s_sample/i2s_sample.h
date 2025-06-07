/**
 * @file audio.h
 * @author Andrea Fato
 * @date 2025-06-03
 * @brief Provides a library audio applicaitons.
 *
 * @copyright (c) 2025 Andrea Fato. Tutti i diritti riservati.
 */

#ifndef I2S_SAMPLE
#define I2S_SAMPLE

/* System */
#include <zephyr/kernel.h>
/* Zephyr peripheral drivers */
#include <zephyr/drivers/i2s.h>

int i2s_config(const struct device *dev_i2s);
int i2s_sample(const struct device *dev_i2s);

#endif /* I2S_SAMPLE */