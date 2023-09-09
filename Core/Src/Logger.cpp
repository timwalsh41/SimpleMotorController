/*
 * Logger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "Logger.h"
#include "SingletonFactory.h"
#include <string>

#ifdef DEBUG
extern MessageHandling::OutputGateway theOutputGateway;
#endif

namespace Logging
{
	Logger::Logger()
	{
		SingletonFactory factory;
		m_pOutputGateway = factory.retrieve<MessageHandling::OutputGateway>();
	}

	void Logger::logString(LogLevel level, const std::string& msg)
	{
#ifdef DEBUG
		std::string logString = std::to_string(m_pTimebase->getTime_ms()) + ": [" + std::string(logLevelString[level]) + "] " + msg;
		m_pOutputGateway->addLogString(logString);
#else
		if (msg.size() > MAX_LOG_QUEUE_SIZE_CHAR)
		{
			std::string errMsg{"Attempted to log message that exceeds queue length"};
			m_logMessageQueue.emplace(m_pTimebase->getTime_ms(), warning, errMsg);
			m_logLength_char += errMsg.size();
		}
		else
		{
			while (m_logLength_char + msg.size() < MAX_LOG_QUEUE_SIZE_CHAR)
			{
				m_logLength_char -= m_logMessageQueue.front().m_message.size();
				m_logMessageQueue.pop();
			}
			m_logMessageQueue.emplace(m_pTimebase->getTime_ms(), level, msg);
			m_logLength_char += msg.size();
		}
#endif
	}

	// return true if there are log messages pending for the host to retrieve
	bool Logger::logMessagesPending()
	{
#ifdef DEBUG
		return false;
#else
		if (m_logMessageQueue.empty())
			return false;

		return true;
#endif
	}

	bool Logger::retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		if (m_logMessageQueue.empty())
		{
			// We send back 2 bytes of 0xFF to signify no log messages are available
			payload.reserve(2);
			payload.push_back(0xFF);
			payload.push_back(0xFF);
		}
		else
		{
			// payload length is 9 bytes plus the message length
			payload.reserve(9 + m_logMessageQueue.front().m_message.size());

			// first two bytes are the number of log messages remaining
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.size() - 1) >> 0));
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.size() - 1) >> 8));

			// next four bytes are the timestamp
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_timestamp) >> 0));
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_timestamp) >> 8));
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_timestamp) >> 16));
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_timestamp) >> 24));

			// next two bytes are the message text length in bytes
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_message.size()) >> 0));
			payload.push_back(static_cast<uint8_t>((m_logMessageQueue.front().m_message.size()) >> 8));

			// and the severity
			payload.push_back(static_cast<uint8_t>(m_logMessageQueue.front().m_severity));

			// finally we copy in the log message itself
			payload.insert(payload.end(), m_logMessageQueue.front().m_message.begin(), m_logMessageQueue.front().m_message.end());

			m_logMessageQueue.pop();
		}

		return true;
	}

} // namespace Logging
