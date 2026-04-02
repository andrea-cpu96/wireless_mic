#include "pcf8574.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define KEYPAD_BTN_NUM 8

struct keypad_handler_t
{
    const struct device *pcf;
    uint8_t buttons_state;
} static keypad_handler;

int pcf8574_config(void)
{
    keypad_handler.pcf = DEVICE_DT_GET(DT_NODELABEL(pcf8574));

    if (!device_is_ready(keypad_handler.pcf))
    {
        printk("PCF8574 not ready\n");
        return -1;
    }

    for (int i = 0; i < KEYPAD_BTN_NUM; i++)
    {
        gpio_pin_configure(keypad_handler.pcf, i, GPIO_INPUT | GPIO_PULL_UP);
    }

    return 0;
}

uint8_t pcf8574_read(void)
{
    keypad_handler.buttons_state = 0;

    for (int i = 0; i < KEYPAD_BTN_NUM; i++)
    {
        keypad_handler.buttons_state |= (!gpio_pin_get(keypad_handler.pcf, i) << i);
    }
    return keypad_handler.buttons_state;
}
