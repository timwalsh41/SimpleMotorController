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
	// Log message payload header size includes:
	// 1 acknowledgement byte
	// 2 number of messages remaining bytes
	// 4 timestamp bytes
	// 2 message length bytes
	// 1 severity byte
	constexpr uint32_t LOG_MESSAGE_PAYLOAD_HEADER_SIZE = 10;	// number of bytes required beyond the 
																// log string itself in a log message
																// payload
	class RealLogger : public BaseLogger
	{
	public:
		RealLogger();

		void loadOutputGateway();
		void logString(LogLevel level, const std::string& msg) override;
		bool retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	private:
		std::shared_ptr<MessageHandling::IOutputGateway> m_pOutputGateway;
		uint32_t m_logLength_char{0};

		bool logStringToMessage(MessageHandling::Message& msg, std::string& logString, LogLevel level);
		bool logStringToPayload(std::vector<uint8_t>& payload, std::string& logString, LogLevel level);
	};

} // namespace Logging

#endif /* SRC_LOGGER_H_ */
