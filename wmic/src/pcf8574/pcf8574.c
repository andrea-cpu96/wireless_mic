#include "pcf8574.h"

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

enum pcf_bank_idx_e
{
    PCF_BTN_BANK_1 = 0,
    PCF_LED_BANK_1,
    PCF_NUM
};

#define KEYPAD_BTN_NUM 8

static int pcf_bank_1_conf(void);
static int pcf_bank_2_conf(void);

struct keypad_handler_t
{
    const struct device *pcf[PCF_NUM];
    gpio_port_value_t buttons_state;
    uint16_t led_state; 
} static keypad_handler;

/**
 * @brief pcf8574_config
 * 
 * @return int 
 */
int pcf8574_config(void)
{
    // Buttons bank
    if(pcf_bank_1_conf() < 0)
    {
        return -1;
    }

    // LEDs bank
    if(pcf_bank_2_conf() < 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief pcf8574_btn_read
 * 
 * @return uint8_t 
 */
uint8_t pcf8574_btn_read(void)
{
    gpio_port_get_raw(keypad_handler.pcf[PCF_BTN_BANK_1], &keypad_handler.buttons_state);
    return (uint8_t)keypad_handler.buttons_state;
}

/**
 * @brief pcf_bank_1_conf
 * 
 * @return int 
 */
static int pcf_bank_1_conf(void)
{
    keypad_handler.pcf[PCF_BTN_BANK_1] = DEVICE_DT_GET(DT_NODELABEL(pcf8574));

    if (!device_is_ready(keypad_handler.pcf[PCF_BTN_BANK_1]))
    {
        printk("PCF8574 not ready\n");
        return -1;
    }

    for (int i = 0; i < KEYPAD_BTN_NUM; i++)
    {
        gpio_pin_configure(keypad_handler.pcf[PCF_BTN_BANK_1], i, GPIO_INPUT | GPIO_PULL_UP);
    }

    return 0;
}

/**
 * @brief pcf_bank_2_conf
 * 
 * @return int 
 */
static int pcf_bank_2_conf(void)
{
    return 0;
}

// INPUTS
// LEDs
// Different PCF
// pcf read dovrebbe dare un solo pin premuto per semplificare il lavoro alla app