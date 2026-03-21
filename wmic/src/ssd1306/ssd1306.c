#include "ssd1306.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include <zephyr/display/cfb.h>

#include <stdio.h>

#define DISPLAY_THREAD_STACK (4096)
K_THREAD_STACK_DEFINE(display_stack, DISPLAY_THREAD_STACK);
K_SEM_DEFINE(display_event, 0, 1);

const struct device *display;

typedef struct display_spec
{
    uint8_t font_width;
    uint8_t font_height;
} display_spec_t;

char dis_str[100];
display_spec_t display_handler;
struct display_capabilities capabilities;
display_event_t ssd1306_event;
display_state_t display_state;

static struct k_thread display_tcb;

static void display_thread(void *a, void *b, void *c);

static void display_process(void);

int ssd1306_config(const struct device *i2c)
{
    display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display))
    {
        return -1;
    }

    k_thread_create(&display_tcb,
                    display_stack,
                    DISPLAY_THREAD_STACK,
                    display_thread,
                    NULL, NULL, NULL,
                    8, 0, K_MSEC(500));

    return 0;
}

void ssd1306_strToShow(char *str)
{
    strcpy(dis_str, str);
}

void ssd1306_turn_on(void)
{
    display_blanking_off(display);
}

void ssd1306_turn_off(void)
{
    display_blanking_on(display);
}

void ssd1306_event_set(display_event_t event)
{
    ssd1306_event = event;
    k_sem_give(&display_event);
}

static void display_thread(void *a, void *b, void *c)
{
    display_get_capabilities(display, &capabilities);

    printf("SSD1306: %dx%d\n",
           capabilities.x_resolution,
           capabilities.y_resolution);

    // Init CFB (Control Frame Buffer)
    if (cfb_framebuffer_init(display))
    {
        printf("CFB init failed\n");
    }

    // Set font 0 (5x8)
    cfb_framebuffer_set_font(display, 0);

    ssd1306_turn_off();

    while (1)
    {
        display_process();
    }
}

static void display_process(void)
{
    static char text[50];

    k_sem_take(&display_event, K_FOREVER);

    switch (ssd1306_event)
    {
    case EV1:
        strcpy(text, "EV1");
        break;
    case EV2:
        strcpy(text, "EV2");
        break;
    case EV3:
        strcpy(text, "EV3");
        break;
    case EV4:
        strcpy(text, "EV4");
        break;
    case SHOW_STRING:
        strcpy(text, dis_str);
        break;
    default:
        break;
    }

    ssd1306_turn_on();

    cfb_framebuffer_clear(display, true);

    cfb_get_font_size(display, 0, &display_handler.font_width, &display_handler.font_height);
    size_t len = strlen(text);

    uint16_t text_width = len * display_handler.font_width;
    uint16_t x = (capabilities.x_resolution - text_width) / 2;
    uint16_t y = 40;

    cfb_print(display, text, x, y);

    // Display update
    cfb_framebuffer_finalize(display);
}