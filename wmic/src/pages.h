#ifndef PAGES_H
#define PAGES_H

#include <stdint.h>
#include "signals.h"

enum pages_e
{
    DEMO_PAGE,
    PEERS_PAGE,
    ADT_PAGE,
    TONE_GEN_PAGE, 
    REC_PAGE,
};

enum buttons_status_e
{
    BUTTON_NONE,
    BUTTON_RIGHT,
    BUTTON_LEFT,
    BUTTON_SET,
};

enum rec_status_e
{
    REC_NONE,
    REC_START,
    REC_READY,
    REC_RUN,
};

// Audio effects data structures
struct adt_settings
{
    uint8_t EnDis;
    uint8_t delay;
    uint8_t fading_lev;
};
struct tone_settings
{
    uint8_t EnDis;
    enum tone_e tone;
};
struct rec_settings
{
    uint8_t EnDis;
    enum rec_status_e track1;
    enum rec_status_e track2;
    enum rec_status_e track3;
    enum rec_status_e track4;
};
typedef struct 
{
    struct adt_settings adt_set;
    struct tone_settings tone_set;
    struct rec_settings rec_set;
} audio_effects_handler_t;

void pages_demo_page(uint8_t EnDis, uint8_t idx, int v1, int v2, int v3, int v4);
void pages_peers_page(enum buttons_status_e button_status, struct bluetooth_peers_struct *peers);
void pages_adt_page(enum buttons_status_e button_status, struct adt_settings *adt_set);
void pages_tones_page(enum buttons_status_e button_status, struct tone_settings *tone_set);
void pages_rec_page(enum buttons_status_e button_status, struct rec_settings *rec_set);
void pages_set_current_page(enum pages_e page);
enum pages_e pages_get_current_page(void);

#endif // PAGES_H
