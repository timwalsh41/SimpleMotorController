/*
 * TestLogger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "TestLogger.h"
#include <iostream>

namespace Logging
{
    TestLogger::TestLogger()
    { 

    }
    
	void TestLogger::logString(LogLevel level, const std::string& msg)
	{
		std::cout << m_pTimebase->getTime_ms() << ": [" << logLevelString[level] << "] " << msg << "\n";
	}

	// this function is not supported, but we need the callback in case we receive the command code
	// basically the retrieve log message command code is used to send out log messages from the motor
	// controller, so we do not require/accept the incoming command
	bool TestLogger::retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		return false;
	}

} // namespace Logging
