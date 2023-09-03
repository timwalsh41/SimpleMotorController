/*
 * TestTimebase.h
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#ifndef INC_TESTTIMEBASE_H_
#define INC_TESTTIMEBASE_H_

#include <ITimebase.h>
#include <chrono>

class TestTimebase: public ITimebase {
public:
	TestTimebase();
	unsigned long getTime_ms() override;
	void delay_ms(unsigned long delayTime_ms) override;
private:
	std::chrono::time_point<std::chrono::steady_clock> m_zeroTime{};
};

#endif /* INC_TESTTIMEBASE_H_ */
