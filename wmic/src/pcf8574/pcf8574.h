#ifndef PCF8574_DRV_H
#define PCF8574_DRV_H

#include <stdint.h>

int pcf8574_config(void);
uint8_t pcf8574_read(void);

#endif // PCF8574_DRV_H