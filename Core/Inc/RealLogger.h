/*
 * RealLogger.h
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include "BaseLogger.h"
#include "ITimebase.h"
#include "IOutputGateway.h"
#include <queue>
#include <memory>

namespace Logging
{
	constexpr uint32_t MAX_LOG_QUEUE_SIZE_CHAR = 4096;

	struct LogMessage
	{
		LogMessage(uint32_t timestamp, LogLevel severity, std::string message) :
			m_timestamp{timestamp},
			m_severity{severity},
			m_message{message}
		{

		}

		uint32_t m_timestamp{0};
		LogLevel m_severity{debug};
		std::string m_message{};
	};

	class RealLogger : public BaseLogger
	{
	public:
		RealLogger();

		void logString(LogLevel level, const std::string& msg) override;
		bool logMessagesPending() override;
		bool retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	private:
		std::shared_ptr<IOutputGateway> m_pOutputGateway;
		std::queue<LogMessage> m_logMessageQueue{};
		uint32_t m_logLength_char{0};
	};

} // namespace Logging

#endif /* SRC_LOGGER_H_ */
