/*
 * STM32TimeBase.h
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#ifndef INC_STM32TIMEBASE_H_
#define INC_STM32TIMEBASE_H_

#include <ITimebase.h>

class STM32Timebase: public ITimebase {
public:
	unsigned long getTime_ms() override;
	void delay_ms(unsigned long delayTime_ms) override;
};

#endif /* INC_STM32TIMEBASE_H_ */
