#ifndef KEYPAD_DRV_H
#define KEYPAD_DRV_H

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

enum led_e
{
    LED_1 = (uint8_t)BIT(0),
    LED_2 = (uint8_t)BIT(1),
    LED_3 = (uint8_t)BIT(2),
    LED_4 = (uint8_t)BIT(3),
    LED_5 = (uint8_t)BIT(4),
    LED_6 = (uint8_t)BIT(5),
    LED_7 = (uint8_t)BIT(6),
    LED_8 = (uint8_t)BIT(7),
    LED_9 = (uint8_t)BIT(8),
};

int keypad_drv_config(void);
uint8_t keypad_drv_btn_read(void);
void keypad_drv_led_clear(uint8_t leds);
void keypad_drv_led_set(uint8_t leds);

#endif // KEYPAD_DRV_H