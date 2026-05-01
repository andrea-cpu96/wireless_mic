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
#include "audio_drv.h"
#include "display_drv.h"
#include "keypad_drv.h"
#include "bluetooth_drv.h"
#include "signals.h"
#include "pages.h"
#if (ENABLE_DSP_FILTER)
#include "low_pass_filter.h"
#endif // ENABLE_DSP_FILTER
#include "adt.h"

const float max = MAX_LIMIT;
const float min = MIN_LIMIT;

// LED data structures
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

enum buttons_status_e
{
    BUTTON_NONE,
    BUTTON_RIGHT,
    BUTTON_LEFT,
    BUTTON_SET,
};

// Buttons state variables
static enum buttons_status_e button_status = BUTTON_NONE;

// Bluetooth peers data structures
const struct bluetooth_peers *peers_p;
static uint8_t peer_idex = 0;
static uint8_t peers_n = 0;
static bool peer_cb_exit = false;

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
static void dsp_adt_init(void);
static void dsp_adt(int32_t *sample);
static int dsp_tone_gen(void);
static void dsp_amplifier(int32_t *sample);

static int gpios_init(void);
static int display_and_keypad(void);
static int bt_init(void);
static int audio_init(void);

static void inputs_handler_cb(void);
static void page_handler(void);
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
        
    // Schedule 100ms work queue
    k_work_schedule(&workq, K_SECONDS(1));

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
#endif // DEBUG_MODE
    k_sleep(K_MSEC(500));

    // App is running
    gpio_pin_set(led.port, led.pin, 1);

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
    if (audio_effects_handler.adt_set.EnDis > 0)
    {
        dsp_adt_init();
    }

    inputs_handler_cb();

    // Pages handler
    page_handler();

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
 * @brief dsp_tone_gen
 *
 * @return int
 */
static int dsp_tone_gen(void)
{
    return 0;
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

    if ((display_drv_config() < 0) || (keypad_drv_config() < 0))
    {
        return -1;
    }

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

    return bluetooth_drv_config(uart0_dev, bt_peer_select, TXRX_MODULE);
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

    return audio_drv_config(i2s_dev, data_elab);
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
        if(audio_effects_handler.tone_set.EnDis > 0)
        {
            pmem[i] = dsp_tone_gen();
            pmem[i+1] = pmem[i];
        }

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
        dsp_adt(&pmem[i]);
    }
#endif // ENABLE_SIGNAL_GEN
}

static uint16_t bt_peer_select(const struct bluetooth_peers *peers, const int16_t *size)
{
    uint8_t selected_peer;

    peer_idex = 0;

    pages_set_current_page(PEERS_PAGE);
    peers_p = peers; // Store peers in a global variable to be used in the page handler and in the inputs handler callback

    // Set a string to be shown onto the display
    display_drv_strToShow(peers_p[peer_idex].name);
    display_drv_event_set(SHOW_STRING);

    while (peer_cb_exit == false)
    {
        k_sleep(K_MSEC(300)); // Gives time to the bluetooth thread to check for other peers
        peers_n = *size;      // Update the number of peers
    }

    selected_peer = peer_idex;
    peer_idex = 0;        // Reset the peer index
    peers_n = 0;          // Reset the number of peers
    peer_cb_exit = false; // Reset the peer callback exit flag
    return selected_peer;
}

/**
 * @brief inputs_handler_cb
 *
 */
static void inputs_handler_cb(void)
{
    enum buttons_e inputs_state = keypad_drv_btn_read();
    button_status = BUTTON_NONE;

    switch (inputs_state)
    {
    case BUTTON_1:
        keypad_drv_led_set(LED_1);
        button_status = BUTTON_RIGHT;
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_2:
        button_status = BUTTON_LEFT;
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_3:
        button_status = BUTTON_SET;
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    case BUTTON_4:
        // Reset the timer
        display_stb_timer = k_uptime_get();
        break;
    default:
        keypad_drv_led_clear(255);
        break;
    }
}

/**
 * @brief page_handler
 *
 */
static void page_handler(void)
{
    switch (pages_get_current_page())
    {
    case DEMO_PAGE:

        break;
    case PEERS_PAGE:
        if (peers_n > 0)
        {
            if (button_status == BUTTON_RIGHT)
            {
                peer_idex = ((peer_idex + 1) % peers_n);
                pages_peers_page(peers_p[peer_idex].name);
            }
            else if (button_status == BUTTON_LEFT)
            {
                peer_idex = (peer_idex == 0) ? (peers_n - 1) : (peer_idex - 1);
                pages_peers_page(peers_p[peer_idex].name);
            }
            else if (button_status == BUTTON_SET)
            {
                pages_set_current_page(ADT_PAGE);
                peer_cb_exit = true; // Exit the peer selection loop in the bluetooth driver
            }
        }
        break;
    case ADT_PAGE:
        if (button_status == BUTTON_RIGHT)
        {
            audio_effects_handler.adt_set.EnDis = 0;
            audio_effects_handler.adt_set.delay = 5;
            audio_effects_handler.adt_set.fading_lev = 0;
            pages_adt_page(audio_effects_handler.adt_set, 0);
        }
        else if (button_status == BUTTON_LEFT)
        {
            audio_effects_handler.adt_set.EnDis = 0;
            audio_effects_handler.adt_set.delay = 5;
            audio_effects_handler.adt_set.fading_lev = 0;
            pages_adt_page(audio_effects_handler.adt_set, 1);
        }
        else if (button_status == BUTTON_SET)
        {
            pages_set_current_page(TONE_GEN_PAGE);
        }

        break;
    case TONE_GEN_PAGE:
        if (button_status == BUTTON_RIGHT)
        {
        }
        else if (button_status == BUTTON_LEFT)
        {
        }
        else if (button_status == BUTTON_SET)
        {
           audio_effects_handler.tone_set.EnDis = 1;
           audio_effects_handler.tone_set.tone = TONE_1KHZ;
        }
        else
        {
           audio_effects_handler.tone_set.EnDis = 0;
           audio_effects_handler.tone_set.tone = TONE_NONE;
        }
         break;
    default:
        break;
    }
}

/**
 * @brief idle_hook
 *
 */
static void display_stb(void)
{
    if (pages_get_current_page() != PEERS_PAGE)
    {

        // Turn off the display after 10s of inactivity
        if (display_drv_get_status() != DISPLAY_OFF)
        {
            if ((k_uptime_get() - display_stb_timer) > DISPLAY_STB_TIME_MS)
            {
                display_drv_turn_off();
            }
        }
        else
        {
            // Do nothing
        }
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
