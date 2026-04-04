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

    gpio_port_set_bits_raw(keypad_handler.pcf[PCF_LED_BANK_1], 255);

    return 0;
}

/**
 * @brief pcf8574_btn_read
 * 
 * @return uint8_t 
 */
enum buttons_e pcf8574_btn_read(void)
{
    gpio_port_get_raw(keypad_handler.pcf[PCF_BTN_BANK_1], &keypad_handler.buttons_state);
    return (enum buttons_e)keypad_handler.buttons_state;
}

/**
 * @brief pcf8574_led_clear
 * 
 * @param leds 
 */
void pcf8574_led_clear(uint8_t leds)
{
    keypad_handler.led_state &= ~leds;
    gpio_port_set_bits_raw(keypad_handler.pcf[PCF_LED_BANK_1], leds);
}

/**
 * @brief pcf8574_led_set
 * 
 * @param leds 
 */
void pcf8574_led_set(uint8_t leds)
{
    keypad_handler.led_state |= leds;
    gpio_port_clear_bits_raw(keypad_handler.pcf[PCF_LED_BANK_1], leds);
}

/**
 * @brief pcf_bank_1_conf
 * 
 * @return int 
 */
static int pcf_bank_1_conf(void)
{
    keypad_handler.pcf[PCF_BTN_BANK_1] = DEVICE_DT_GET(DT_NODELABEL(pcf8574_btn_b1));

    if (!device_is_ready(keypad_handler.pcf[PCF_BTN_BANK_1]))
    {
        printk("PCF8574 bank_1 not ready\n");
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
    keypad_handler.pcf[PCF_LED_BANK_1] = DEVICE_DT_GET(DT_NODELABEL(pcf8574_led_b1));

    if (!device_is_ready(keypad_handler.pcf[PCF_LED_BANK_1]))
    {
        printk("PCF8574 bank_2 not ready\n");
        return -1;
    }

    for (int i = 0; i < KEYPAD_BTN_NUM; i++)
    {
        gpio_pin_configure(keypad_handler.pcf[PCF_LED_BANK_1], i, GPIO_OUTPUT);
    }

    return 0;
}