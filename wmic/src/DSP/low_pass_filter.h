/*
 * low_pass_filter.h
 *
 *  Created on: Dec 17, 2025
 *      Author: andre
 */

#ifndef LOW_PASS_FILTER_H_
#define LOW_PASS_FILTER_H_

#include <arm_math.h>

#define MAX_BLOCK_LEN 16

void lowpass_filter_init(uint16_t block_len);
void lowpass_filter_exc(q15_t *input, q15_t *out);

#endif /* LOW_PASS_FILTER_H_ */
