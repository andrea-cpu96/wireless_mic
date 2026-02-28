/*
 * adt.h - Artificial Dual Track Effect
 *
 *  Created on: Feb 22, 2026
 *      Author: andrea
 */

#ifndef ADT_H_
#define ADT_H_

#include <arm_math.h>
#include <stdint.h>

void adt_init(uint16_t delay_ms);
void adt_store_sample(int32_t sample);
int32_t adt_get_sample(void);

#endif /* ADT_H_ */
