/*
 * Utility.h
 *
 *  Created on: Jun 14, 2023
 *      Author: tim
 */

#ifndef INC_UTILITY_H_
#define INC_UTILITY_H_

#include "stdint.h"

namespace Utility
{
	void delayCycles(uint32_t);
	uint32_t timeElapsed(uint32_t, uint32_t);

} /* namespace Utility */

#endif /* INC_UTILITY_H_ */
