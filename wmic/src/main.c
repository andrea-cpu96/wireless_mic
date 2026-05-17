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
#include "veeprom.h"
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

#define WORKQ_PERIOD_MS 100

#define SAMPLES_IN_1S 44100
#define SAMPLES_IN_25MS 1100

#define ADDRESSES_IN_1S (SAMPLES_IN_1S / SAMPLES_IN_25MS)
#define WORQ_CYCLES_1S_REC ADDRESSES_IN_1S

const float max = MAX_LIMIT;
const float min = MIN_LIMIT;

// LED data structures
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_NODELABEL(led1), gpios);

// Buttons state variables
static enum buttons_status_e button_status = BUTTON_NONE;

// Bluetooth peers data structures
struct bluetooth_peers_struct bluetooth_peers_handler = {
    .peers_p = NULL,
    .peer_idex = 0,
    .peers_n = 0,
    .peer_cb_exit = false};

static int64_t display_stb_timer = 0;

#if (TEST_REC)
int16_t rec_data[SAMPLES_IN_1S] = {0};
int rec_data_index = 0;
uint32_t mem_address_track1[ADDRESSES_IN_1S] = {0};
uint32_t mem_address_track2[ADDRESSES_IN_1S] = {0};
int mem_id_track1 = 0;
int mem_id_track2 = 0;
volatile int16_t rec_sample = 0;
#endif // TEST_REC

// I2S data structures
const struct device *i2s_dev = DEVICE_DT_GET(DT_NODELABEL(i2s0));

// UART data structures
const struct device *uart0_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

// I2C data structures
const struct device *i2c1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

// Audio effects data structures
static audio_effects_handler_t audio_effects_handler = {
    .adt_set = {.EnDis = 0, .delay = 0, .fading_lev = 0},
    .tone_set = {.EnDis = 0, .tone = TONE_NONE},
    .rec_set = {.EnDis = 0, .track1 = REC_NONE, .track2 = REC_NONE, .track3 = REC_NONE, .track4 = REC_NONE}};

static void workq_100ms(struct k_work *work);

#if (ENABLE_DSP_FILTER)
static void dsp_filter_init();
static void dsp_filter(int32_t *sample);
#endif // ENABLE_DSP_FILTER
static void dsp_adt_init(void);
static void dsp_adt(int32_t *sample);
static void dsp_tone_gen(int32_t *sample);
static void dsp_rec(int32_t *sample, int size);
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
    veeprom_init();

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
    static int16_t *rec_data_store1 = rec_data;
    static int16_t *rec_data_store2 = rec_data;

    // ADT init
    if (audio_effects_handler.adt_set.EnDis > 0)
    {
        dsp_adt_init();
    }

    // Inputs handler
    inputs_handler_cb();

    // Pages handler
    page_handler();

    // Display standby after long inactivity
    display_stb();

#if (TEST_REC)
    /*
     * 44100 samples = 88200 Bytes = 1S
     * 1100 samples = 2200 Bytes about 25ms
     *
     * It will take about 4s to save 1s recording
     */
    if (audio_effects_handler.rec_set.EnDis > 0)
    {
        if ((mem_id_track1 < WORQ_CYCLES_1S_REC) &&
            (audio_effects_handler.rec_set.track1 == REC_READY))
        {
            mem_address_track1[mem_id_track1] = veeprom_write(rec_data_store1, (SAMPLES_IN_25MS * sizeof(int16_t)));
            mem_id_track1++;
            rec_data_store1 = (rec_data_store1 + SAMPLES_IN_25MS);
        }
        else if ((mem_id_track2 < WORQ_CYCLES_1S_REC) &&
                 (audio_effects_handler.rec_set.track2 == REC_READY))
        {
            mem_address_track2[mem_id_track2] = veeprom_write(rec_data_store2, (SAMPLES_IN_25MS * sizeof(int16_t)));
            mem_id_track2++;
            rec_data_store2 = (rec_data_store2 + SAMPLES_IN_25MS);
        }
    }
#endif // TEST_REC

    k_work_schedule(&workq, K_MSEC(WORKQ_PERIOD_MS));
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
static void dsp_filter(int32_t *sample)
{
    float32_t data_f32 = 0.0;
    q15_t data_q15;
    q15_t out;
    int32_t filtered;

#if (ENABLE_STEREO_DIFF)
    data_f32 = ((sample[0]) / (float32_t)2147483648); // Normalization from int32 to float32 (range -1.0 to 1.0)
    arm_float_to_q15(&data_f32, &data_q15, 1);        // Conversion from float32 to 15
    lowpass_filter_exc(&data_q15, &out);
    filtered = (int32_t)(out * (65536)); // Conversion from q15 to int32 (2147483648 / 32768 = 65536)
    sample[0] = filtered;                // Left channel
    sample[1] = filtered;                // Right channel (equal to left)
#else
    // Left channel
    data_f32 = ((sample[0]) / (float32_t)2147483648);
    arm_float_to_q15(&data_f32, &data_q15, 1);
    lowpass_filter_exc(&data_q15, &out);
    sample[0] = (int32_t)(out * (2147483648 / 32768));

    // Right channel
    data_f32 = ((sample[1]) / (float32_t)2147483648);
    arm_float_to_q15(&data_f32, &data_q15, 1);
    lowpass_filter_exc(&data_q15, &out);
    sample[1] = (int32_t)(out * (2147483648 / 32768));
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
 */
static void dsp_tone_gen(int32_t *sample)
{
    int32_t tone_samp = (int32_t)(signals_get_sample(audio_effects_handler.tone_set.tone) *
                                  (float32_t)22767); // Conversion from float32 (range -1.0 to 1.0) to int16

    /*
     * Shift to upper 16 bits (according to bluetooth module data format)
     * Reduced to 10 bits shift to reduce amplification
     */
    tone_samp = (tone_samp << 10);
    sample[0] += tone_samp;
    sample[1] -= tone_samp;
}

/**
 * @brief dsp_rec
 *
 * @param sample
 * @param size
 */
static void dsp_rec(int32_t *sample, int size)
{
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
#if (TEST_REC)
    static int id = 0;
    static int first1 = 1;
    static int first2 = 1;
#endif // TEST_REC

    int samples_2ch_num = (block_size / sizeof(int32_t)); // Number of 32 bits samples in 2 channels
    int size_16b_1ch = (block_size / 2);                  // Size of 1 channel 16 bits per sample

#if (ENABLE_SIGNAL_GEN)
    for (int i = 0; i < samples_2ch_num - 1; i += 2)
    {
        pmem[i] = (int32_t)(signals_get_sample() * (float32_t)22767); // Conversion from float32 (range -1.0 to 1.0) to int16
        pmem[i] = (pmem[i] << 16);                                    // Shift to upper 16 bits (according to bluetooth module data format)
        pmem[i + 1] = pmem[i];                                        // Right channel equal to left channel
#if (ENABLE_DSP_FILTER)
        dsp_filter(&pmem[i]);
#endif // ENABLE_DSP_FILTER
    }
#else
/*
    if (first1 && (audio_effects_handler.rec_set.EnDis > 0) &&
        (audio_effects_handler.rec_set.track1 == REC_RUN))
    {
        first2 = 1;
        if ((id < mem_id_track1) &&
            (audio_effects_handler.rec_set.track1 == REC_RUN))
        {
            memset((int16_t *)(rec_data+id*2200), 0, 4400); // Clear the recording buffer before writing new data
            veeprom_read(mem_address_track1[id], (rec_data+id*2200), 4400);
            id += 2;
        }
        else if (id >= mem_id_track1)
        {
            id = 0;
            first1 = 0;
        }
    }
    if (first2 && (audio_effects_handler.rec_set.EnDis > 0) &&
        (audio_effects_handler.rec_set.track2 == REC_RUN))
    {
        first1 = 1;
        if ((id < mem_id_track2) &&
            (audio_effects_handler.rec_set.track2 == REC_RUN))
        {
            memset((int16_t *)(rec_data+id*2200), 0, 4400); // Clear the recording buffer before writing new data
            veeprom_read(mem_address_track2[id], (rec_data+id*2200), 4400);
            id += 2;
        }
        else if (id >= mem_id_track2)
        {
            id = 0;
            first2 = 0;
        }
    }
*/

    for (int i = 0; i < samples_2ch_num - 1; i += 2)
    {
#if (TEST_REC)
        if ((rec_data_index < SAMPLES_IN_1S) &&
            audio_effects_handler.rec_set.track1 == REC_RUN)
        {
            veeprom_read(mem_address_track1[0] + rec_data_index*sizeof(int16_t), &rec_sample, sizeof(int16_t));
            pmem[i] = (rec_sample << 16);
            pmem[i + 1] = pmem[i];
            rec_data_index++;
            continue;
        }
        else if ((rec_data_index < SAMPLES_IN_1S) &&
                 audio_effects_handler.rec_set.track2 == REC_RUN)
        {
            veeprom_read(mem_address_track2[0]+rec_data_index*sizeof(int16_t), &rec_sample, sizeof(int16_t));
            pmem[i] = (rec_sample << 16);
            pmem[i + 1] = pmem[i];
            rec_data_index++;
            continue;
        }
        else if (rec_data_index >= SAMPLES_IN_1S)
        {
            rec_data_index = 0;
            return;
        }
#endif // TEST_REC

        if (audio_effects_handler.tone_set.EnDis > 0)
        {
            dsp_tone_gen(&pmem[i]);
        }

        if ((pmem[i] <= max) && (pmem[i] >= min))
        {
            dsp_amplifier(&pmem[i]);
            dsp_amplifier(&pmem[i + 1]);
        }
#if (ENABLE_STEREO_DIFF)
        int32_t diff = (pmem[i + 1] - pmem[i]); // right - left
        pmem[i] = diff;
        pmem[i + 1] = diff;
#endif // ENABLE_STEREO_DIFF
#if (ENABLE_DSP_FILTER)
        dsp_filter(&pmem[i]);
#endif // ENABLE_DSP_FILTER
        if (audio_effects_handler.adt_set.EnDis > 0)
        {
            dsp_adt(&pmem[i]);
        }
#if (TEST_REC)
        if ((audio_effects_handler.rec_set.EnDis > 0))
        {
            if ((rec_data_index < SAMPLES_IN_1S) &&
                (audio_effects_handler.rec_set.track1 == REC_START))
            {
                rec_data[rec_data_index] = (int16_t)(pmem[i] >> 16);
                rec_data_index++;
            }
            else if ((rec_data_index < SAMPLES_IN_1S) &&
                     (audio_effects_handler.rec_set.track2 == REC_START))
            {
                rec_data[rec_data_index] = (int16_t)(pmem[i] >> 16);
                rec_data_index++;
            }
        }
#endif // TEST_REC
    }
#endif // ENABLE_SIGNAL_GEN
}

/**
 * @brief bt_peer_select
 *
 * @param peers
 * @param size
 * @return uint16_t
 */
static uint16_t bt_peer_select(const struct bluetooth_peers *peers, const int16_t *size)
{
    uint8_t selected_peer;

    bluetooth_peers_handler.peer_idex = 0;

    pages_set_current_page(PEERS_PAGE);
    bluetooth_peers_handler.peers_p = peers; // Store peers in a global variable to be used in the page handler and in the inputs handler callback

    // Set a string to be shown onto the display
    display_drv_strToShow(bluetooth_peers_handler.peers_p[bluetooth_peers_handler.peer_idex].name);
    display_drv_event_set(SHOW_STRING);

    while (bluetooth_peers_handler.peer_cb_exit == false)
    {
        k_sleep(K_MSEC(300));                    // Gives time to the bluetooth thread to check for other peers
        bluetooth_peers_handler.peers_n = *size; // Update the number of peers
        bluetooth_drv_at_send("SCAN=1");
    }

    selected_peer = bluetooth_peers_handler.peer_idex;
    bluetooth_peers_handler.peer_idex = 0;        // Reset the peer index
    bluetooth_peers_handler.peers_n = 0;          // Reset the number of peers
    bluetooth_peers_handler.peer_cb_exit = false; // Reset the peer callback exit flag
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

    if (inputs_state != BUTTON_NO)
    {
        // Turn on the display
        display_drv_turn_on();

        // Reset the timer
        display_stb_timer = k_uptime_get();
    }

    switch (inputs_state)
    {
    case BUTTON_5:
        keypad_drv_led_set(LED_6);
        button_status = BUTTON_RIGHT;
        break;
    case BUTTON_7:
        keypad_drv_led_set(LED_8);
        button_status = BUTTON_LEFT;
        break;
    case BUTTON_6:
        keypad_drv_led_set(LED_1);
        button_status = BUTTON_SET;
        break;
    case BUTTON_4:
        keypad_drv_led_set(LED_7);
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
        pages_peers_page(button_status, &bluetooth_peers_handler);
        break;
    case ADT_PAGE:
        pages_adt_page(button_status, &audio_effects_handler.adt_set);
        break;
    case TONE_GEN_PAGE:
        pages_tones_page(button_status, &audio_effects_handler.tone_set);
        break;
    case REC_PAGE:
        pages_rec_page(button_status, &audio_effects_handler.rec_set);
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
