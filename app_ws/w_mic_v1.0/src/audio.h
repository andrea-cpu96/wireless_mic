/* System */
#include <zephyr/kernel.h>
/* Zephyr peripheral drivers */
#include <zephyr/drivers/i2s.h>

int i2s_config(const struct device *dev_i2s);
int i2s_sample(const struct device *dev_i2s);