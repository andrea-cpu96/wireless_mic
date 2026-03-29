#include "ssd1306.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include <zephyr/display/cfb.h>

#include <stdio.h>

#include "my_fonts.h"

#define DISPLAY_THREAD_STACK (4096)
#define DISPLAY_THREAD_PRIORITY 8

K_THREAD_STACK_DEFINE(display_stack, DISPLAY_THREAD_STACK);
K_SEM_DEFINE(display_event_sem, 0, 1);

// Display data structures
struct display_handler_t
{
    const struct device *display;
    struct display_capabilities capabilities;
    uint8_t font_width;
    uint8_t font_height;
    char dis_str[100];
    display_pages_t page;
    display_state_t display_state;
} static display_handler;
static display_event_t ssd1306_event;
static int my_font_idx = 0;

static struct k_thread display_tcb;

static void display_thread(void *a, void *b, void *c);

static void display_process(void);
static void show_str(void);
static void show_page(void);

/**
 * @brief ssd1306_config
 *
 * @return int
 */
int ssd1306_config(void)
{
    display_handler.display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_handler.display))
    {
        return -1;
    }

    k_thread_create(&display_tcb,
                    display_stack,
                    DISPLAY_THREAD_STACK,
                    display_thread,
                    NULL, NULL, NULL,
                    DISPLAY_THREAD_PRIORITY, 0, K_NO_WAIT);

    return 0;
}

/**
 * @brief ssd1306_strToShow
 *
 * @param str
 */
void ssd1306_strToShow(const char *str)
{
    strcpy(display_handler.dis_str, str);
}

/**
 * @brief ssd1306_turn_on
 *
 */
void ssd1306_turn_on(void)
{
    display_blanking_off(display_handler.display);
    display_handler.display_state = DISPLAY_ON;
}

/**
 * @brief ssd1306_turn_off
 *
 */
void ssd1306_turn_off(void)
{
    display_blanking_on(display_handler.display);
    display_handler.display_state = DISPLAY_OFF;
}

/**
 * @brief ssd1306_event_set
 *
 * @param event
 */
void ssd1306_event_set(display_event_t event)
{
    ssd1306_event = event;
    k_sem_give(&display_event_sem);
}

/**
 * @brief ssd1306_get_status
 *
 * @return display_state_t
 */
display_state_t ssd1306_get_status(void)
{
    return display_handler.display_state;
}

/**
 * @brief ssd1306_pageToShow
 *
 * @param page
 */
void ssd1306_pageToShow(display_pages_t page)
{
    strcpy(display_handler.page.title, page.title);
    display_handler.page.par_select = page.par_select;
    display_handler.page.EnDis = page.EnDis;
    for (int i = 0; i < 4; i++)
    {
        strcpy(display_handler.page.par[i].title, page.par[i].title);
        strcpy(display_handler.page.par[i].val, page.par[i].val);
    }
}

/**
 * @brief display_thread
 *
 * @param a
 * @param b
 * @param c
 */
static void display_thread(void *a, void *b, void *c)
{
    display_get_capabilities(display_handler.display, &display_handler.capabilities);

    printf("SSD1306: %dx%d\n",
           display_handler.capabilities.x_resolution,
           display_handler.capabilities.y_resolution);

    // Init CFB (Control Frame Buffer)
    if (cfb_framebuffer_init(display_handler.display))
    {
        printf("CFB init failed\n");
    }

    // Get my font index
    my_font_idx = cfb_get_numof_fonts(display_handler.display) - 1;

    // Set font 0
    cfb_framebuffer_set_font(display_handler.display, 0);

    // Read font sizes
    cfb_get_font_size(display_handler.display, 0, &display_handler.font_width, &display_handler.font_height);

    // Turn of the display
    ssd1306_turn_off();

    while (1)
    {
        display_process();
    }
}

/**
 * @brief display_process
 *
 */
static void display_process(void)
{
    k_sem_take(&display_event_sem, K_FOREVER);

    switch (ssd1306_event)
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
        show_str();
        break;
    case SHOW_PAGE:
        show_page();
        break;
    default:
        break;
    }

    ssd1306_turn_on();
}

/**
 * @brief show_str
 *
 */
static void show_str(void)
{
    cfb_framebuffer_clear(display_handler.display, true);

    size_t len = strlen(display_handler.dis_str);

    uint16_t text_width = len * display_handler.font_width;
    uint16_t x = (display_handler.capabilities.x_resolution - text_width) / 2;
    uint16_t y = 40;

    cfb_print(display_handler.display, display_handler.dis_str, x, y);

    // Display update
    cfb_framebuffer_finalize(display_handler.display);
}

/**
 * @brief show_page
 *
 */
static void show_page(void)
{
    struct cfb_position p1 = {.x = 3, .y = 16};
    struct cfb_position p2 = {.x = 7, .y = 19};

    if (display_handler.page.par_select == 1)
    {
        p1.x = 3;
        p1.y = 16;
        p2.x = 7;
        p2.y = 19;
    }
    else if (display_handler.page.par_select == 2)
    {
        p1.x = 3;
        p1.y = 25;
        p2.x = 7;
        p2.y = 28;
    }
    else if (display_handler.page.par_select == 3)
    {
        p1.x = 67;
        p1.y = 16;
        p2.x = 71;
        p2.y = 19;
    }
    else if (display_handler.page.par_select == 4)
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

    cfb_framebuffer_clear(display_handler.display, true);

    cfb_framebuffer_set_font(display_handler.display, 0);
    cfb_print(display_handler.display, display_handler.page.title, 40, 0);
    cfb_framebuffer_finalize(display_handler.display);

    cfb_framebuffer_set_font(display_handler.display, my_font_idx);

    cfb_draw_rect(display_handler.display, &p1, &p2);

    if (display_handler.page.EnDis == 0)
    {
        cfb_print(display_handler.display, "OFF", 100, 3);
    }
    else
    {
        cfb_print(display_handler.display, "ON", 110, 3);
    }

    cfb_print(display_handler.display, display_handler.page.par[0].title, 10, 15);
    cfb_print(display_handler.display, display_handler.page.par[1].title, 10, 24);
    cfb_print(display_handler.display, display_handler.page.par[2].title, 75, 15);
    cfb_print(display_handler.display, display_handler.page.par[3].title, 75, 24);

    cfb_print(display_handler.display, display_handler.page.par[0].val, 45, 15);
    cfb_print(display_handler.display, display_handler.page.par[1].val, 45, 24);
    cfb_print(display_handler.display, display_handler.page.par[2].val, 110, 15);
    cfb_print(display_handler.display, display_handler.page.par[3].val, 110, 24);

    cfb_framebuffer_finalize(display_handler.display);
}