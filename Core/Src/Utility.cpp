/*
 * Utility.cpp
 *
 *  Created on: Jun 14, 2023
 *      Author: tim
 */

#include "stm32l4xx_hal.h"
#include "Utility.h"

namespace Utility
{
	// delay for a number of NOP cycles
	// each cycle is 150 ns at 80 MHz clock, meaning each cycle takes
	// 150 ns * 80 MHz = 12 clock cycles
	void delayCycles(uint32_t numCycles)
	{
		for (uint32_t i = 0; i < numCycles; ++i)
			__NOP();
	}

} /* namespace Utility */
