#ifndef PCF8574_DRV_H
#define PCF8574_DRV_H

#include <zephyr/kernel.h>

#include <stdint.h>

enum buttons_e
{
    BUTTON_1 = (uint8_t)~BIT(0),
    BUTTON_2 = (uint8_t)~BIT(1),
    BUTTON_3 = (uint8_t)~BIT(2),
    BUTTON_4 = (uint8_t)~BIT(3),
    BUTTON_5 = (uint8_t)~BIT(4),
    BUTTON_6 = (uint8_t)~BIT(5),
    BUTTON_7 = (uint8_t)~BIT(6),
    BUTTON_8 = (uint8_t)~BIT(7),
    BUTTON_9 = (uint8_t)~BIT(8),
};

int pcf8574_config(void);
uint8_t pcf8574_btn_read(void);

#endif // PCF8574_DRV_H