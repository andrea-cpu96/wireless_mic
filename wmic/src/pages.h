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
typedef struct 
{
    struct adt_settings adt_set;
    struct tone_settings tone_set;
} audio_effects_handler_t;

void pages_demo_page(uint8_t EnDis, uint8_t idx, int v1, int v2, int v3, int v4);
void pages_peers_page(const char *peer_name);
void pages_adt_page(struct adt_settings adt_set, uint8_t idx);
void pages_tones_page(struct tone_settings tone_set);
void pages_set_current_page(enum pages_e page);
enum pages_e pages_get_current_page(void);

#endif // PAGES_H
