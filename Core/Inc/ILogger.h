/*
 * ILogger.h
 *
 *  Created on: Sep 6, 2023
 *      Author: tim
 */

#ifndef INC_ILOGGER_H_
#define INC_ILOGGER_H_

#include "Message.h"
#include "ITimebase.h"
#include "SingletonFactory.h"
#include <string>
#include <vector>
#include <memory>

enum LogLevel {
	debug = 0,
	info,
	warning,
	error,
	fatal
};

namespace Logging
{

	class ILogger
	{
	public:
		ILogger()
		{
			SingletonFactory factory;
			m_pTimebase = factory.retrieve<ITimebase>();
		}

		virtual ~ILogger() {}
		virtual void logString(LogLevel level, const std::string& msg) = 0;
		virtual bool logMessagesPending() = 0;
		virtual bool retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
	protected:
		static constexpr std::string_view logLevelString[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
		std::shared_ptr<ITimebase> m_pTimebase;
	};

} // namespace Logging

#endif /* INC_ILOGGER_H_ */
