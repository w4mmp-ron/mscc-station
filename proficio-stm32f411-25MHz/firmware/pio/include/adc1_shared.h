/**
 * Shared ADC1 for die temp sensor + PA NTC (PA4 / ADC1_IN4).
 */
#ifndef ADC1_SHARED_H
#define ADC1_SHARED_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

void     adc1_shared_init(void);
/** One-shot conversion on channel; returns 12-bit raw or 0xFFFFFFFF on failure. */
uint32_t adc1_shared_read(uint32_t channel);

#endif /* ADC1_SHARED_H */
