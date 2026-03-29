#ifndef PAGES_H
#define PAGES_H

#include <stdint.h>

// Audio effects data structures
struct adt_settings
{
    uint8_t EnDis;
    uint8_t delay;
    uint8_t fading_lev;
};
typedef struct 
{
    struct adt_settings adt_set;
} audio_effects_handler_t;

void pages_demo_page(uint8_t EnDis, uint8_t idx, int v1, int v2, int v3, int v4);
void pages_adt_page(struct adt_settings adt_set, uint8_t idx);

#endif // PAGES_H