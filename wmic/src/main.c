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
#include "pcf8574.h"
#include "bt1036c_drv.h"
#include "signals.h"
#include "pages.h"
#if (ENABLE_DSP_FILTER)
#include "low_pass_filter.h"
#endif // ENABLE_DSP_FILTER
#if (ENABLE_DSP_ADT_EFFECT)
#include "adt.h"
#endif // ENABLE_DSP_ADT_EFFECT

const float max = MAX_LIMIT;
const float min = MIN_LIMIT;

// LED data structures
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

#if (ENABLE_INPUTS_INT)
#define INPUTS_N 4
// Inputs data structures
const struct gpio_dt_spec inputs[INPUTS_N] = {GPIO_DT_SPEC_GET(DT_NODELABEL(input1), gpios),
                                              GPIO_DT_SPEC_GET(DT_NODELABEL(input2), gpios),
                                              GPIO_DT_SPEC_GET(DT_NODELABEL(input3), gpios),
                                              GPIO_DT_SPEC_GET(DT_NODELABEL(input4), gpios)};
static struct gpio_callback inputs_cb;
#endif // ENABLE_INPUTS_INT
uint8_t right;
uint8_t left;
uint8_t set;

static int64_t display_stb_timer = 0;

// I2S data structures
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));

// UART data structures
const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

// I2C data structures
const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

// Audio effects data structures
static audio_effects_handler_t audio_effects_handler;

static void workq_100ms(struct k_work *work);

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
static int display_and_keypad(void);
static int bt_init(void);
static int audio_init(void);

static void inputs_handler_cb(void);
static void data_elab(int32_t *pmem, uint32_t block_size);
static uint16_t bt_peer_select(const struct bluetooth_peers *peers, const int16_t *size);

static void display_stb(void);

static void system_fault_handler(void);

K_WORK_DELAYABLE_DEFINE(workq, workq_100ms);

int main(void)
{
    // Filter init
#if (ENABLE_DSP_FILTER)
    dsp_filter_init();
#endif // ENABLE_DSP_FILTER

    // GPIOS init
    if (gpios_init() != 0)
    {
        printk("GPIO init failed, resetting...\n");
        system_fault_handler();
    }

    // display init
    if (display_and_keypad() != 0)
    {
        printk("Display and keypad init failed, resetting...\n");
        system_fault_handler();
    }
#if (!DEBUG_MODE)
    // Bluetooth init
    if (bt_init() != 0)
    {
        printk("Bluetooth init failed, resetting...\n");
    }

    // Audio init
    if (audio_init() != 0)
    {
        printk("Audio init failed, resetting...\n");
        system_fault_handler();
    }
#endif //DEBUG_MODE
    k_sleep(K_MSEC(500));

    // App is running
    gpio_pin_set(led.port, led.pin, 1);

    // Schedule 100ms work queue
    k_work_schedule(&workq, K_SECONDS(1));

    while (1)
    {
        k_sleep(K_FOREVER);
    }
    return 0;
}

/**
 * @brief workq_100ms
 *
 * @param work
 */
static void workq_100ms(struct k_work *work)
{
    // ADT init
#if (ENABLE_DSP_ADT_EFFECT)
    if (audio_effects_handler.adt_set.EnDis > 0)
    {
        dsp_adt_init();
    }
#endif // ENABLE_DSP_ADT_EFFECT

#if (!ENABLE_INPUTS_INT)
    inputs_handler_cb();
#endif // ENABLE_INPUTS_INT

    display_stb();
    k_work_schedule(&workq, K_MSEC(100));
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
    uint16_t delay = (audio_effects_handler.adt_set.delay * 100);
    adt_init(delay);
}

/**
 * @brief dsp_adt
 *
 * @param sample
 */
static void dsp_adt(int32_t *sample)
{
    adt_store_sample(sample[0]);
    sample[1] = (adt_get_sample() >> audio_effects_handler.adt_set.fading_lev);
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
    // Signaling LED
    if (!gpio_is_ready_dt(&led))
    {
        return -1;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

#if (ENABLE_INPUTS_INT)
    int inputs_bit = 0;
    // Inputs
    for (int i = 0; i < INPUTS_N; i++)
    {
        inputs_bit |= BIT(inputs[i].pin);

        if (!gpio_is_ready_dt(&inputs[i]))
        {
            return -1;
        }
        gpio_pin_configure_dt(&inputs[i], GPIO_INPUT);
        gpio_pin_interrupt_configure_dt(&inputs[i], GPIO_INT_EDGE_TO_ACTIVE);
    }

    gpio_init_callback(&inputs_cb, inputs_handler_cb, inputs_bit);
    gpio_add_callback(inputs[0].port, &inputs_cb);
#endif // ENABLE_INPUTS_INT

    return 0;
}

/**
 * @brief display_and_keypad
 *
 * @return int
 */
static int display_and_keypad(void)
{
    // Check device is ready
    if (!device_is_ready(i2c1_dev))
    {
        printf("I2C device not ready\n");
        return -1;
    }

#if (DEBUG_MODE)
    if (pcf8574_config() < 0)
    {
        return -1;
    }
#else
    if ((ssd1306_config() < 0) || (pcf8574_config() < 0))
    {
        return -1;
    }
#endif // DEBUG_MODE
    return 0;
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
 * @brief inputs_handler_cb
 *
 */
static void inputs_handler_cb(void)
{
    enum buttons_e inputs_state = pcf8574_btn_read();
    right = 0;
    left = 0;
    set = 0;

    switch (inputs_state)
    {
    case BUTTON_1:
        pcf8574_led_set(LED_1);
        right = 1;
        audio_effects_handler.adt_set.EnDis = 0;
        audio_effects_handler.adt_set.delay = 5;
        audio_effects_handler.adt_set.fading_lev = 0;
        pages_adt_page(audio_effects_handler.adt_set, 0);
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_2:
        left = 1;
        audio_effects_handler.adt_set.EnDis = 0;
        audio_effects_handler.adt_set.delay = 5;
        audio_effects_handler.adt_set.fading_lev = 0;
        pages_adt_page(audio_effects_handler.adt_set, 1);
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_3:
        set = 1;
        audio_effects_handler.adt_set.EnDis = 0;
        audio_effects_handler.adt_set.delay = 5;
        audio_effects_handler.adt_set.fading_lev = 0;
        pages_adt_page(audio_effects_handler.adt_set, 2);
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_4:
        audio_effects_handler.adt_set.EnDis = 1;
        audio_effects_handler.adt_set.delay = 5;
        audio_effects_handler.adt_set.fading_lev = 0;
        pages_adt_page(audio_effects_handler.adt_set, 0);
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    default:
        pcf8574_led_clear(255);
        break;
    }
}

/**
 * @brief idle_hook
 *
 */
static void display_stb(void)
{
    // Turn off the display after 10s of inactivity
    if (ssd1306_get_status() != DISPLAY_OFF)
    {
        if ((k_uptime_get() - display_stb_timer) > DISPLAY_STB_TIME_MS)
        {
            ssd1306_turn_off();
        }
    }
    else
    {
        // Do nothing
    }
}

/**
 * @brief system_fault_handler
 *
 */
static void system_fault_handler(void)
{
    k_sleep(K_MSEC(500));
    sys_reboot(SYS_REBOOT_COLD);
}