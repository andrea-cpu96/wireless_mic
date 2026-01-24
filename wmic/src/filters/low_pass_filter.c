/*
 * low_pass_filter.c
 *
 *  Created on: Dec 17, 2025
 *      Author: andre
 */

#include "low_pass_filter.h"
#include "signals.h"

#define FILTER_TAPS 32
#define IMP_RSP_LENGTH 31

const float f32_LP_1HZ_2HZ_IMPULSE_RESPONSE[IMP_RSP_LENGTH] = {0.0};

static uint16_t filter_block_len = 1; // Number of samples processed at time
static q15_t q15_LP_1HZ_2HZ_IMPULSE_RESPONSE[IMP_RSP_LENGTH]; // q15 version of the filter impulse response
static q15_t lowpadd_filter_state[FILTER_TAPS + MAX_BLOCK_LEN]; // Buffer containing space for the passed FILTER_TAPS samples plus the current input samples
static arm_fir_instance_q15 lowpass_filter_instance;

void lowpass_filter_init(uint16_t block_len)
{
	if (block_len > MAX_BLOCK_LEN)
	{
		return;
	}

	filter_block_len = block_len; 
	arm_float_to_q15(f32_LP_1HZ_2HZ_IMPULSE_RESPONSE, q15_LP_1HZ_2HZ_IMPULSE_RESPONSE, IMP_RSP_LENGTH);
	arm_fir_init_q15(&lowpass_filter_instance, FILTER_TAPS, q15_LP_1HZ_2HZ_IMPULSE_RESPONSE, lowpadd_filter_state, filter_block_len);
}

void lowpass_filter_exc(q15_t *input, q15_t *out)
{
	arm_fir_q15(&lowpass_filter_instance, input, out, filter_block_len);
}
