/*
 * MockLogger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "MockLogger.h"
#include <iostream>

namespace Logging
{
    MockLogger::MockLogger()
    { 

    }
    
	void MockLogger::logString(LogLevel level, const std::string& msg)
	{
		std::cout << m_pTimebase->getTime_ms() << ": [" << logLevelString[level] << "] " << msg << "\n";
	}
	
	bool MockLogger::logMessagesPending()
	{
	    return false;
    }
    
    bool MockLogger::retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
    {
        return true;
    }

} // namespace Logging
