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

#define HZ_5_SIG_LEN 301
#define KHZ1_15_SIG_LEN 320
#define IMP_RSP_LENGTH	31

extern float LP_1HZ_2HZ_IMPULSE_RESPONSE[IMP_RSP_LENGTH];
extern float32_t input_signal_f32_1kHz_15kHz[KHZ1_15_SIG_LEN];

#endif /* SIGNALS_H_ */
