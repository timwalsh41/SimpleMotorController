/*
 * Logger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "Logger.h"
#include "OutputGateway.h"

extern MessageHandling::OutputGateway theOutputGateway;

namespace Logging
{

	void Logger::logString(LogLevel level, const std::string& msg)
	{
		theOutputGateway.logString(level, msg);
	}

} // namespace Logging
