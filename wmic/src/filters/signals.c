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

/*Low pass filter : Fpass 1hz Fstop 3hz Fs 100hz Order 31*/

float32_t input_signal_f32_1kHz_15kHz[KHZ1_15_SIG_LEN] =
    {};

#endif
