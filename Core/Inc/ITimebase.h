/*
 * ITimebase.h
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#ifndef INC_ITIMEBASE_H_
#define INC_ITIMEBASE_H_

class ITimebase {
public:
	virtual ~ITimebase() {};
	virtual unsigned long getTime_ms() = 0;
	virtual void delay_ms(unsigned long delayTime_ms) = 0;
};

#endif /* INC_ITIMEBASE_H_ */
