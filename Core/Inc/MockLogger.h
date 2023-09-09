/*
 * MockLogger.h
 *
 *  Created on: Sep 6, 2023
 *      Author: tim
 */

#ifndef INC_MOCKLOGGER_H_
#define INC_MOCKLOGGER_H_

#include "ILogger.h"
#include "ITimebase.h"
#include <map>
#include <string>
#include <memory>

namespace Logging
{
	class MockLogger : public ILogger
	{
	public:
		MockLogger();

		void logString(LogLevel level, const std::string& msg) override;
		bool logMessagesPending() override;
		bool retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	};

} // namespace Logging



#endif /* INC_MOCKLOGGER_H_ */
