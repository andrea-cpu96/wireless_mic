/*
 * Display driver
 * OLED Controller: SSD1306
 * Author: Andrea Fato
 * Rev: 1.0
 * Date: 05/04/2026
 */

#include "display_drv.h"

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>

#include <stdio.h>

#include "my_fonts.h"

#define DISPLAY_DRV_THREAD_STACK (4096)
#define DISPLAY_DRV_THREAD_PRIORITY 8

K_THREAD_STACK_DEFINE(display_drv_stack, DISPLAY_DRV_THREAD_STACK);
K_SEM_DEFINE(display_drv_event_sem, 0, 1);

// Display data structures
struct display_drv_handler_t
{
    const struct device *display;
    struct display_capabilities capabilities;
    uint8_t font_width;
    uint8_t font_height;
    char dis_str[100];
    display_pages_t page;
    display_state_t display_state;
} static display_drv_handler;
static display_event_t display_drv_event;
static int my_font_idx = 0;

static struct k_thread display_drv_tcb;

static void display_drv_thread(void *a, void *b, void *c);

static void display_drv_process(void);
static void display_drv_show_str(void);
static void display_drv_show_page(void);

/**
 * @brief display_drv_config
 *
 * @return int
 */
int display_drv_config(void)
{
    display_drv_handler.display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_drv_handler.display))
    {
        return -1;
    }

    k_thread_create(&display_drv_tcb,
                    display_drv_stack,
                    DISPLAY_DRV_THREAD_STACK,
                    display_drv_thread,
                    NULL, NULL, NULL,
                    DISPLAY_DRV_THREAD_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief display_drv_strToShow
 *
 * @param str
 */
void display_drv_strToShow(const char *str)
{
    strcpy(display_drv_handler.dis_str, str);
}

/**
 * @brief display_drv_turn_on
 *
 */
void display_drv_turn_on(void)
{
    display_blanking_off(display_drv_handler.display);
    display_drv_handler.display_state = DISPLAY_ON;
}

/**
 * @brief display_drv_turn_off
 *
 */
void display_drv_turn_off(void)
{
    display_blanking_on(display_drv_handler.display);
    display_drv_handler.display_state = DISPLAY_OFF;
}

/**
 * @brief display_drv_event_set
 *
 * @param event
 */
void display_drv_event_set(display_event_t event)
{
    display_drv_event = event;
    k_sem_give(&display_drv_event_sem);
}

/**
 * @brief display_drv_get_status
 *
 * @return display_state_t
 */
display_state_t display_drv_get_status(void)
{
    return display_drv_handler.display_state;
}

/**
 * @brief display_drv_pageToShow
 *
 * @param page
 */
void display_drv_pageToShow(display_pages_t page)
{
    strcpy(display_drv_handler.page.title, page.title);
    display_drv_handler.page.par_select = page.par_select;
    display_drv_handler.page.EnDis = page.EnDis;
    for (int i = 0; i < 4; i++)
    {
        strcpy(display_drv_handler.page.par[i].title, page.par[i].title);
        strcpy(display_drv_handler.page.par[i].val, page.par[i].val);
    }
}

/**
 * @brief display_drv_thread
 *
 * @param a
 * @param b
 * @param c
 */
static void display_drv_thread(void *a, void *b, void *c)
{
    display_get_capabilities(display_drv_handler.display, &display_drv_handler.capabilities);

    printf("SSD1306: %dx%d\n",
           display_drv_handler.capabilities.x_resolution,
           display_drv_handler.capabilities.y_resolution);

    // Init CFB (Control Frame Buffer)
    if (cfb_framebuffer_init(display_drv_handler.display))
    {
        printf("CFB init failed\n");
    }

    // Get my font index
    my_font_idx = cfb_get_numof_fonts(display_drv_handler.display) - 1;

    // Set font 0
    cfb_framebuffer_set_font(display_drv_handler.display, 0);

    // Read font sizes
    cfb_get_font_size(display_drv_handler.display, 0, &display_drv_handler.font_width, &display_drv_handler.font_height);

    // Turn of the display
    display_drv_turn_off();

    while (1)
    {
        display_drv_process();
    }
}

/**
 * @brief display_drv_process
 *
 */
static void display_drv_process(void)
{
    k_sem_take(&display_drv_event_sem, K_FOREVER);

    switch (display_drv_event)
    {
    case EV1:
        break;
    case EV2:
        break;
    case EV3:
        break;
    case EV4:
        break;
    case SHOW_STRING:
        display_drv_show_str();
        break;
    case SHOW_PAGE:
        display_drv_show_page();
        break;
    default:
        break;
    }

    display_drv_turn_on();
}

/**
 * @brief display_drv_show_str
 *
 */
static void display_drv_show_str(void)
{
    cfb_framebuffer_clear(display_drv_handler.display, true);

    size_t len = strlen(display_drv_handler.dis_str);

    uint16_t text_width = len * display_drv_handler.font_width;
    uint16_t x = (display_drv_handler.capabilities.x_resolution - text_width) / 2;
    uint16_t y = 40;

    cfb_print(display_drv_handler.display, display_drv_handler.dis_str, x, y);

    // Display update
    cfb_framebuffer_finalize(display_drv_handler.display);
}

/**
 * @brief display_drv_show_page
 *
 */
static void display_drv_show_page(void)
{
    struct cfb_position p1 = {.x = 3, .y = 16};
    struct cfb_position p2 = {.x = 7, .y = 19};

    if (display_drv_handler.page.par_select == 1)
    {
        p1.x = 3;
        p1.y = 16;
        p2.x = 7;
        p2.y = 19;
    }
    else if (display_drv_handler.page.par_select == 2)
    {
        p1.x = 3;
        p1.y = 25;
        p2.x = 7;
        p2.y = 28;
    }
    else if (display_drv_handler.page.par_select == 3)
    {
        p1.x = 67;
        p1.y = 16;
        p2.x = 71;
        p2.y = 19;
    }
    else if (display_drv_handler.page.par_select == 4)
    {
        p1.x = 67;
        p1.y = 25;
        p2.x = 71;
        p2.y = 28;
    }
    else
    {
        // Title
        p1.x = 34;
        p1.y = 5;
        p2.x = 38;
        p2.y = 9;
    }

    cfb_framebuffer_clear(display_drv_handler.display, true);

    cfb_framebuffer_set_font(display_drv_handler.display, 0);
    cfb_print(display_drv_handler.display, display_drv_handler.page.title, 40, 0);
    cfb_framebuffer_finalize(display_drv_handler.display);

    cfb_framebuffer_set_font(display_drv_handler.display, my_font_idx);

    cfb_draw_rect(display_drv_handler.display, &p1, &p2);

    if (display_drv_handler.page.EnDis == 0)
    {
        cfb_print(display_drv_handler.display, "OFF", 100, 3);
    }
    else
    {
        cfb_print(display_drv_handler.display, "ON", 110, 3);
    }

    cfb_print(display_drv_handler.display, display_drv_handler.page.par[0].title, 10, 15);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[1].title, 10, 24);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[2].title, 75, 15);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[3].title, 75, 24);

    cfb_print(display_drv_handler.display, display_drv_handler.page.par[0].val, 45, 15);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[1].val, 45, 24);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[2].val, 110, 15);
    cfb_print(display_drv_handler.display, display_drv_handler.page.par[3].val, 110, 24);

    cfb_framebuffer_finalize(display_drv_handler.display);
}