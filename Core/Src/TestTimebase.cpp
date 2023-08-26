/*
 * TestTimebase.cpp
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 *
 *  Note that threading is not supported for the arm-none-eabi compilers (makes sense as threads
 *  require an OS), so this is for non-base metal environments only
 */

#include <TestTimebase.h>
#include <chrono>
#include <thread>

unsigned long TestTimebase::getTime_ms()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void TestTimebase::delay_ms(unsigned long delayTime_ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(delayTime_ms));
}
