#ifndef AUDIO_DRV_H
#define AUDIO_DRV_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>

#include <math.h>
#include <limits.h>

typedef void (*pi2s_elab)(int32_t *pmem, uint32_t block_size); // Callback function for data elaboration

int audio_drv_config(const struct device *i2s_dev, pi2s_elab cb);

#endif /* AUDIO_DRV_H */