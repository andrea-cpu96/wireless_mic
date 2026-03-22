#ifndef SSD1306_DRV_H
#define SSD1306_DRV_H

#include <zephyr/device.h>

typedef enum
{
    EV1,
    EV2,
    EV3,
    EV4,
    SHOW_STRING,
} display_event_t;

typedef enum
{
    IDLE = 0,
} display_state_t;

int ssd1306_config(void);
void ssd1306_strToShow(const char *str);
void ssd1306_turn_on(void);
void ssd1306_turn_off(void);
void ssd1306_event_set(display_event_t event);

#endif /* SSD1306_DRV_H */