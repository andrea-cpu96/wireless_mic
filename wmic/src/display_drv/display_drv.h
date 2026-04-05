#ifndef DISPLAY_DRV_H
#define DISPLAY_DRV_H

#include <stdint.h>

typedef enum
{
    EV1,
    EV2,
    EV3,
    EV4,
    SHOW_STRING,
    SHOW_PAGE,
} display_event_t;

typedef enum
{
    DISPLAY_OFF = 0,
    DISPLAY_ON,
    IDLE,
} display_state_t;

typedef struct 
{
    char title[10];
    char val[5];
} page_data_par_t;

typedef struct
{
    char title[10];
    uint8_t EnDis;
    page_data_par_t par[4]; 
    uint8_t par_select;
} display_pages_t;

int display_drv_config(void);
void display_drv_strToShow(const char *str);
void display_drv_turn_on(void);
void display_drv_turn_off(void);
void display_drv_event_set(display_event_t event);
display_state_t display_drv_get_status(void);
void display_drv_pageToShow(display_pages_t page);

#endif // DISPLAY_DRV_H