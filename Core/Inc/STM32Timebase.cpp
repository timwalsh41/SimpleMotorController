/*
 * STM32TimeBase.cpp
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#include <STM32Timebase.h>
#include "stm32l4xx_hal.h"

unsigned long STM32Timebase::getTime_ms()
{
	return static_cast<unsigned long>(HAL_GetTick());
}

void STM32Timebase::delay_ms(unsigned long delayTime_ms)
{
	HAL_Delay(static_cast<uint32_t>(delayTime_ms));
}
