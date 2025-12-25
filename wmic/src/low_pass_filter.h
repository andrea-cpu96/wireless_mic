/*
 * low_pass_filter.h
 *
 *  Created on: Dec 17, 2025
 *      Author: andre
 */

#ifndef LOW_PASS_FILTER_H_
#define LOW_PASS_FILTER_H_

#include <arm_math.h>

void lowpass_filter_init(void);
q15_t lowpass_filter_exc(q15_t *input);

#endif /* LOW_PASS_FILTER_H_ */
