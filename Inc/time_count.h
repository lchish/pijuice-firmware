/*
 * time_count.h
 *
 *  Created on: 12.12.2016.
 *      Author: milan
 */

#ifndef TIME_COUNT_H_
#define TIME_COUNT_H_

#include "stdint.h"
#include "stm32f0xx_hal.h"

#define TICK_PERIOD_MS		20

//#define TIME_COUNTERS_MAX 	16

#define MS_TIME_COUNTER_INIT(c)	(c=HAL_GetTick())
#define MS_TIME_COUNT(c)	(HAL_GetTick()-c)

//extern uint32_t ticks[TIME_COUNTERS_MAX];

//int8_t AddTimeCounter();
void TimeTickCb(uint16_t periodMs);

/**
 * @brief  Delays for amount of micro seconds
 * @param  micros: Number of microseconds for delay
 * @retval None
 */
__STATIC_INLINE void DelayUs(__IO uint32_t micros) {
#if 0//!defined(STM32F0xx)
	uint32_t start = DWT->CYCCNT;

	/* Go to number of cycles for system */
	micros *= (HAL_RCC_GetHCLKFreq() / 1000000);

	/* Delay till end */
	while ((DWT->CYCCNT - start) < micros);
#else
	/* Go to clock cycles */
	micros *= (SystemCoreClock * 16 / 1000000) / 6;
	micros >>= 4;

	// Wait till done
	volatile uint32_t dcnt = micros;
	while (dcnt--);
#endif
}

#endif /* TIME_COUNT_H_ */
