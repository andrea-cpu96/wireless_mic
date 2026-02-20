/*
 * signals.c
 *
 *  Created on: Dec 10, 2025
 *      Author: andre
 */

#ifndef __SIGNALS__H
#define __SIGNALS__H

#include <arm_math.h>
#include "signals.h"

float32_t gen_signal[SIG_GEN_LEN] = {0x0000efff};

float32_t signals_get_sample(int index)
{
    if (index >= SIG_GEN_LEN)
    {
        return 0;
    }

    return gen_signal[index];
}

#endif
