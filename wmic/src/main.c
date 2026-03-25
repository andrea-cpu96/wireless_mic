/*
*⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣶⣶⣦⣄⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣷⣦⡀⠀⠀⠀⠀⠀⠀
*⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣷⣤⠀⠈⠙⢿⣿⣿⣿⣿⣿⣦⡀⠀⠀⠀⠀
*⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣿⣿⣿⠆⠰⠶⠀⠘⢿⣿⣿⣿⣿⣿⣆⠀⠀⠀
*⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣼⣿⣿⣿⠏⠀⢀⣠⣤⣤⣀⠙⣿⣿⣿⣿⣿⣷⡀⠀
*⠀⠀⠀⠀⠀⠀⠀⠀⢠⠋⢈⣉⠉⣡⣤⢰⣿⣿⣿⣿⣿⣷⡈⢿⣿⣿⣿⣿⣷⡀
*⠀⠀⠀⠀⠀⠀⠀⡴⢡⣾⣿⣿⣷⠋⠁⣿⣿⣿⣿⣿⣿⣿⠃⠀⡻⣿⣿⣿⣿⡇
*⠀⠀⠀⠀⠀⢀⠜⠁⠸⣿⣿⣿⠟⠀⠀⠘⠿⣿⣿⣿⡿⠋⠰⠖⠱⣽⠟⠋⠉⡇
*⠀⠀⠀⠀⡰⠉⠖⣀⠀⠀⢁⣀⠀⣴⣶⣦⠀⢴⡆⠀⠀⢀⣀⣀⣉⡽⠷⠶⠋⠀
*⠀⠀⠀⡰⢡⣾⣿⣿⣿⡄⠛⠋⠘⣿⣿⡿⠀⠀⣐⣲⣤⣯⠞⠉⠁⠀⠀⠀⠀⠀
*⠀⢀⠔⠁⣿⣿⣿⣿⣿⡟⠀⠀⠀⢀⣄⣀⡞⠉⠉⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀
*⠀⡜⠀⠀⠻⣿⣿⠿⣻⣥⣀⡀⢠⡟⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*⢰⠁⠀⡤⠖⠺⢶⡾⠃⠀⠈⠙⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*⠈⠓⠾⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*/

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>

#include <stdio.h>
#include <arm_math.h>

#include "config.h"
#include "audio.h"
#include "ssd1306.h"
#include "bt1036c_drv.h"
#include "signals.h"
#if (ENABLE_DSP_FILTER)
#include "low_pass_filter.h"
#endif // ENABLE_DSP_FILTER
#if (ENABLE_DSP_ADT_EFFECT)
#include "adt.h"
#endif // ENABLE_DSP_ADT_EFFECT

#define USER_BUTTONS_N 4

const float max = MAX_LIMIT;
const float min = MIN_LIMIT;

// LED data structures
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

// User buttons data structures
const struct gpio_dt_spec ubtn[USER_BUTTONS_N] = {GPIO_DT_SPEC_GET(DT_NODELABEL(button1), gpios),
                                                  GPIO_DT_SPEC_GET(DT_NODELABEL(button2), gpios),
                                                  GPIO_DT_SPEC_GET(DT_NODELABEL(button3), gpios),
                                                  GPIO_DT_SPEC_GET(DT_NODELABEL(button4), gpios)};
static struct gpio_callback buttons_cb;
uint8_t right;
uint8_t left;
uint8_t set;

// I2S data structures
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));

// UART data structures
const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

// I2C data structures
const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

#if (ENABLE_DSP_FILTER)
static void dsp_filter_init();
static void dsp_filter(int32_t *pmem);
#endif // ENABLE_DSP_FILTER
#if (ENABLE_DSP_ADT_EFFECT)
static void dsp_adt_init(void);
static void dsp_adt(int32_t *sample);
#endif // ENABLE_DSP_ADT_EFFECT
static void dsp_amplifier(int32_t *sample);

static int gpios_init(void);
static int display_init(void);
static int bt_init(void);
static int audio_init(void);

static void buttons_handler_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void data_elab(int32_t *pmem, uint32_t block_size);
static uint16_t bt_peer_select(const struct bluetooth_peers *peers, const int16_t *size);

int main(void)
{
    // Filter init
#if (ENABLE_DSP_FILTER)
    dsp_filter_init();
#endif // ENABLE_DSP_FILTER

    // ADT init
#if (ENABLE_DSP_ADT_EFFECT)
    dsp_adt_init();
#endif // ENABLE_DSP_ADT_EFFECT

    // GPIOS init
    if (gpios_init() != 0)
    {
        printk("GPIO init failed, resetting...\n");
        sys_reboot(SYS_REBOOT_COLD);
    }

    // display init
    if (display_init() != 0)
    {
        printk("Display init failed, resetting...\n");
        sys_reboot(SYS_REBOOT_COLD);
    }

    // Bluetooth init
    if (bt_init() != 0)
    {
        printk("Bluetooth init failed, resetting...\n");
        sys_reboot(SYS_REBOOT_COLD);
    }

    // Audio init
    if (audio_init() != 0)
    {
        printk("Audio init failed, resetting...\n");
        sys_reboot(SYS_REBOOT_COLD);
    }

    k_sleep(K_MSEC(500));

    // App is running
    gpio_pin_set(led.port, led.pin, 1);

    while (1)
    {
        k_sleep(K_FOREVER);
    }
    return 0;
}

#if (ENABLE_DSP_FILTER)
/**
 * @brief dsp_filter_init
 *
 * @return void
 */
static void dsp_filter_init(void)
{
    lowpass_filter_init(1); // block_len = 1
    return;
}

/**
 * @brief dsp_filter
 *
 * @return void
 */
static void dsp_filter(int32_t *pmem)
{
    float32_t data_f32 = 0.0;
    q15_t data_q15;
    q15_t out;
    int32_t filtered;

#if (ENABLE_STEREO_DIFF)
    data_f32 = ((pmem[0]) / (float32_t)2147483648); // Normalization from int32 to float32 (range -1.0 to 1.0)
    arm_float_to_q15(&data_f32, &data_q15, 1);      // Conversion from float32 to 15
    lowpass_filter_exc(&data_q15, &out);
    filtered = (int32_t)(out * (65536)); // Conversion from q15 to int32 (2147483648 / 32768 = 65536)
    pmem[0] = filtered;                  // Left channel
    pmem[1] = filtered;                  // Right channel (equal to left)
#else
    // Left channel
    data_f32 = ((pmem[0]) / (float32_t)2147483648);
    arm_float_to_q15(&data_f32, &data_q15, 1);
    lowpass_filter_exc(&data_q15, &out);
    pmem[0] = (int32_t)(out * (2147483648 / 32768));

    // Right channel
    data_f32 = ((pmem[1]) / (float32_t)2147483648);
    arm_float_to_q15(&data_f32, &data_q15, 1);
    lowpass_filter_exc(&data_q15, &out);
    pmem[1] = (int32_t)(out * (2147483648 / 32768));
#endif // ENABLE_STEREO_DIFF

    return;
}
#endif // ENABLE_DSP_FILTER

#if (ENABLE_DSP_ADT_EFFECT)
/**
 * @brief dsp_adt_init
 *
 */
static void dsp_adt_init(void)
{
    adt_init(100);
}

/**
 * @brief dsp_adt
 *
 * @param sample
 */
static void dsp_adt(int32_t *sample)
{
    adt_store_sample(sample[0]);
    sample[1] = (adt_get_sample() >> 2);
}
#endif // ENABLE_DSP_ADT_EFFECT

/**
 * @brief dsp_amplifier
 *
 * @param sample
 */
static void dsp_amplifier(int32_t *sample)
{
    uint32_t voice = ((*sample) < 0) ? -(*sample) : (*sample);

    if (voice < 200)
    {
        *sample = 0;
    }
    else
    {
        *sample <<= AMP_FACTOR;
    }

    return;
}

/**
 * @brief gpios_init
 *
 * @return int
 */
static int gpios_init(void)
{
    int ubtn_bit = 0;

    // Signaling LED
    if (!gpio_is_ready_dt(&led))
    {
        return -1;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

    // User buttons
    for (int i = 0; i < USER_BUTTONS_N; i++)
    {
        ubtn_bit |= BIT(ubtn[i].pin);

        if (!gpio_is_ready_dt(&ubtn[i]))
        {
            return -1;
        }
        gpio_pin_configure_dt(&ubtn[i], GPIO_INPUT);
        gpio_pin_interrupt_configure_dt(&ubtn[i], GPIO_INT_EDGE_TO_ACTIVE);
    }

    gpio_init_callback(&buttons_cb, buttons_handler_cb, ubtn_bit);
    gpio_add_callback(ubtn[0].port, &buttons_cb);

    return 0;
}

/**
 * @brief display_init
 *
 * @return int
 */
static int display_init(void)
{
    // Check device is ready
    if (!device_is_ready(i2c1_dev))
    {
        printf("I2C device not ready\n");
        return -1;
    }

    return ssd1306_config();
}

/**
 * @brief bt_init
 *
 * @return int
 */
static int bt_init(void)
{
    // Check device is ready
    if (!device_is_ready(uart0_dev))
    {
        printf("UART device not ready\n");
        return -1;
    }

    return bt1036c_config(uart0_dev, bt_peer_select, TXRX_MODULE);
}

/**
 * @brief audio_init
 *
 * @return int
 */
static int audio_init(void)
{
    // Check device is ready
    if (!device_is_ready(i2s_dev))
    {
        printf("I2S device not ready\n");
        return -1;
    }

    return audio_config(i2s_dev, data_elab);
}

/**
 * @brief data_elab
 *
 * @return void
 */
static void data_elab(int32_t *pmem, uint32_t block_size)
{
    int size = block_size / sizeof(int32_t);

#if (ENABLE_SIGNAL_GEN)
    for (int i = 0; i < size - 1; i += 2)
    {
        pmem[i] = (int32_t)(signals_get_sample() * (float32_t)22767); // Conversion from float32 (range -1.0 to 1.0) to int16
        pmem[i] = (pmem[i] << 16);                                    // Shift to upper 16 bits (according to bluetooth module data format)
        pmem[i + 1] = pmem[i];                                        // Right channel equal to left channel
#if (ENABLE_DSP_FILTER)
        dsp_filter(&pmem[i]);
#endif // ENABLE_DSP_FILTER
    }
#else
    for (int i = 0; i < size - 1; i += 2)
    {
        if ((pmem[i] <= max) && (pmem[i] >= min))
        {
            dsp_amplifier(&pmem[i]);
            dsp_amplifier(&pmem[i + 1]);
        }
#if (ENABLE_STEREO_DIFF)
        int32_t diff = pmem[i + 1] - pmem[i]; // right - left
        pmem[i] = diff;
        pmem[i + 1] = diff;
#endif // ENABLE_STEREO_DIFF
#if (ENABLE_DSP_FILTER)
        dsp_filter(&pmem[i]);
#endif // ENABLE_DSP_FILTER
#if (ENABLE_DSP_ADT_EFFECT)
        dsp_adt(&pmem[i]);
#endif
    }
#endif // ENABLE_SIGNAL_GEN
}

static uint16_t bt_peer_select(const struct bluetooth_peers *peers, const int16_t *size)
{
    uint8_t peer_idex = 0;
    uint8_t peers_n = 0;

    while (set == 0)
    {
        peers_n = *size;

        // Set a string to be shown onto the display
        ssd1306_strToShow(peers[0].name);
        ssd1306_event_set(SHOW_STRING);

        k_sleep(K_MSEC(300)); // Gives time to the bluetooth thread to check for other peers

        if (right)
        {
            right = 0;
            peer_idex = ((peer_idex + 1) % peers_n);
        }
        else if (left)
        {
            left = 0;
            peer_idex = (peer_idex == 0) ? (peers_n - 1) : (peer_idex - 1);
        }
    }

    set = 0;

    return peer_idex;
}

/**
 * @brief buttons_handler_cb
 *
 * @param dev
 * @param cb
 * @param pins
 */
static void buttons_handler_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    right = 0;
    left = 0;
    set = 0;

    if (pins == BIT(23))
    {
        right = 1;
    }
    else if (pins == BIT(24))
    {
        left = 1;
    }
    else if (pins == BIT(8))
    {
        set = 1;
    }
    else if (pins == BIT(9))
    {
    }
}
