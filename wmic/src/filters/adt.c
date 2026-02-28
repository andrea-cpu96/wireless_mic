/*
 * adt.c - Artificial Dual Track Effect
 *
 *  Created on: Feb 22, 2026
 *      Author: andrea
 */

#include "adt.h"
#include <math.h>
#include <string.h>

#define ADT_BUFF_SIZE   50000

int32_t adt_buff[ADT_BUFF_SIZE] = {0};

static uint32_t sample_offset = 0;
static uint16_t index_high = 0; 
static uint16_t index_low = 0; 

void adt_init(uint16_t delay_ms)
{
    sample_offset = ((delay_ms * 44100) / 1000);
}

void adt_store_sample(int32_t sample)
{
    adt_buff[index_high] = sample;
    index_high = ((index_high + 1) % ADT_BUFF_SIZE);
}

int32_t adt_get_sample(void)
{
    int32_t sample = 0; 

    if(((index_high - index_low + ADT_BUFF_SIZE) % ADT_BUFF_SIZE) >= sample_offset)
    {
        sample =  adt_buff[index_low];
        index_low = ((index_low + 1) % ADT_BUFF_SIZE);
    }

    return sample; 
}
