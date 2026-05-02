/*
 * signals.h
 *
 *  Created on: Dec 10, 2025
 *      Author: andre
 */

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <stdint.h>
#include "arm_math.h"

#define SIG_GEN_LEN 441

enum tone_e
{
    TONE_NONE,
    TONE_500HZ,
    TONE_1KHZ,
    TONE_3KHZ,
};

float32_t signals_get_sample(enum tone_e tone);

#endif /* SIGNALS_H_ */
