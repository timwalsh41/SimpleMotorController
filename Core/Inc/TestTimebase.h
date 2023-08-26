/*
 * TestTimebase.h
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#ifndef INC_TESTTIMEBASE_H_
#define INC_TESTTIMEBASE_H_

#include <ITimebase.h>

class TestTimebase: public ITimebase {
public:
	unsigned long getTime_ms() override;
	void delay_ms(unsigned long delayTime_ms) override;
};

#endif /* INC_TESTTIMEBASE_H_ */
