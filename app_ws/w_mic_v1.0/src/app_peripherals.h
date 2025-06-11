#ifndef APP_PERIPHERALS
#define APP_PERIPHERALS

/* System */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
/* Peripheral drivers */
#include "i2s_drv.h"
#include "adc_drv.h"

#define NUM_OF_BUFFERS 2

#define STEREO 1 // MAX98357A always exspects input data in stereo format (it selects the channnel via  SD pin)
#if (STEREO == 1)
#define CHANNELS_NUMBER 2
#else
#define CHANNELS_NUMBER 1
#endif

#define SAMPLE_FREQ 44100

/* I2S data structures */
extern const struct device *i2s_dev;
extern i2s_drv_config_t hi2s;

/* ADC data structures */
extern const struct device *adc;
extern adc_handler_t hadc;

int adc_config(void);
int i2s_config(void);

#endif /* APP_PERIPHERALS */
