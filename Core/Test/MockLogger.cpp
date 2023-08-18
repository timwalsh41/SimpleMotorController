/*
 * Logger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "Logger.h"
#include <iostream>

Logging::Logger theLogger;

namespace Logging
{
	void Logger::logString(LogLevel level, const std::string& msg)
	{
		std::cout << msg << "\n";
	}

} // namespace Logging
