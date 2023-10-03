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
	
	bool TestLogger::logMessagesPending()
	{
	    return false;
    }
    
    bool TestLogger::retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
    {
        return true;
    }

} // namespace Logging
