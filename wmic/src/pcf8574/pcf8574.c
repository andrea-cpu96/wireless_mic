#include "pcf8574.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

struct keypad_handler_t
{
    const struct device *pcf;
    uint8_t pin[8];
} static keypad_handler;

int pcf8574_config(void)
{
    keypad_handler.pcf = DEVICE_DT_GET(DT_NODELABEL(pcf8574));

    if (!device_is_ready(keypad_handler.pcf))
    {
        printk("PCF8574 non pronto!\n");
        return -1;
    }

    gpio_pin_configure(keypad_handler.pcf, 0, GPIO_INPUT | GPIO_PULL_UP);

    return 0;
}

uint8_t pcf8574_read(void)
{
    return gpio_pin_get(keypad_handler.pcf, 0);
}
