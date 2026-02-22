/*
 * low_pass_filter.c
 *
 *  Created on: Dec 17, 2025
 *      Author: andre
 */

#include "low_pass_filter.h"
#include "signals.h"

#define FILTER_TAPS 40
#define IMP_RSP_LENGTH FILTER_TAPS

const float f32_LP_IMPULSE_RESPONSE[IMP_RSP_LENGTH] = {
  0.000757790927f, -0.000272106641f, -0.000464868761f, 0.00161574199f, -0.00317860465f, 0.00485364441f, -0.00600290904f, 0.00574720697f, 
  -0.00320280157f, -0.00218722341f, 0.0103106787f, -0.0201319512f, 0.029606808f, -0.035794083f, 0.0351014361f, -0.0234512724f, 
  -0.00420638639f, 0.0570563264f, -0.16628401f, 0.620126605f, 0.620126605f, -0.16628401f, 0.0570563264f, -0.00420638639f, 
  -0.0234512724f, 0.0351014361f, -0.035794083f, 0.029606808f, -0.0201319512f, 0.0103106787f, -0.00218722341f, -0.00320280157f, 
  0.00574720697f, -0.00600290904f, 0.00485364441f, -0.00317860465f, 0.00161574199f, -0.000464868761f, -0.000272106641f, 0.000757790927f
};

static uint16_t filter_block_len = 1;							// Number of samples processed at time
static q15_t q15_LP_IMPULSE_RESPONSE[IMP_RSP_LENGTH];	// q15 version of the filter impulse response
static q15_t lowpadd_filter_state[FILTER_TAPS + MAX_BLOCK_LEN]; // Buffer containing space for the passed FILTER_TAPS samples plus the current input samples
static arm_fir_instance_q15 lowpass_filter_instance;

void lowpass_filter_init(uint16_t block_len)
{
	if (block_len > MAX_BLOCK_LEN)
	{
		return;
	}

	filter_block_len = block_len;
	arm_float_to_q15(f32_LP_IMPULSE_RESPONSE, q15_LP_IMPULSE_RESPONSE, IMP_RSP_LENGTH);
	arm_fir_init_q15(&lowpass_filter_instance, FILTER_TAPS, q15_LP_IMPULSE_RESPONSE, lowpadd_filter_state, filter_block_len);
}

void lowpass_filter_exc(q15_t *input, q15_t *out)
{
	arm_fir_q15(&lowpass_filter_instance, input, out, filter_block_len);
}
