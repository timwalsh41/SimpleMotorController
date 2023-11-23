/*
 * RealLogger.cpp
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#include "RealLogger.h"
#include "SingletonFactory.h"
#include "CommandInterface.h"
#include <string>

namespace Logging
{
	RealLogger::RealLogger()
	{

	}

	void RealLogger::loadOutputGateway()
	{
		// we have a circular dependency between the logger and the output gateway, to solve this we create the logger first,
		// then create the gateway, then load up the logger's pointer to the gateway singleton here
		SingletonFactory factory;
		m_pOutputGateway = factory.retrieve<MessageHandling::IOutputGateway>();
	}

	void RealLogger::logString(LogLevel level, const std::string& inputString)
	{
		MessageHandling::Message msg;
		std::string outputString = std::to_string(m_pTimebase->getTime_ms()) + ": [" + std::string(logLevelString[level]) + "] " + inputString + "\r\n";
		
		// if we can allocate memory for the log string (e.g. logStringToMessage succeeds) we'll send it, otherwise there's nothing we can do...
		if (logStringToMessage(msg, outputString, level))
			m_pOutputGateway->sendMessage(msg);
	}

	bool RealLogger::logStringToMessage(MessageHandling::Message& msg, std::string& logString, LogLevel level)
	{
		std::vector<uint8_t> payload;

		if (!logStringToPayload(payload, logString, level))
			return false;

		msg.setCommandCode(CMD_GET_LOG_MESSAGE);
		msg.setAck();
		msg.setPayload(std::move(payload));
		msg.setCRC();

		return true;
	}
			
	bool RealLogger::logStringToPayload(std::vector<uint8_t>& payload, std::string& logString, LogLevel level)
	{
		payload.clear();

		// payload length is 9 bytes plus the message length
		payload.reserve(LOG_MESSAGE_PAYLOAD_HEADER_SIZE + logString.size());

		if (payload.capacity() < LOG_MESSAGE_PAYLOAD_HEADER_SIZE + logString.size())
			return false;

		// first byte is the acknowledgement
		payload.push_back(ACK_SUCCESSFUL);

		// next two bytes are the number of log messages remaining
		payload.push_back(static_cast<uint8_t>(0));
		payload.push_back(static_cast<uint8_t>(0));

		// next four bytes are the timestamp
		uint32_t timestamp = m_pTimebase->getTime_ms();
		payload.push_back(static_cast<uint8_t>(timestamp >> 0));
		payload.push_back(static_cast<uint8_t>(timestamp >> 8));
		payload.push_back(static_cast<uint8_t>(timestamp >> 16));
		payload.push_back(static_cast<uint8_t>(timestamp >> 24));

		// next two bytes are the message text length in bytes
		payload.push_back(static_cast<uint8_t>(logString.size() >> 0));
		payload.push_back(static_cast<uint8_t>(logString.size() >> 8));

		// and the severity
		payload.push_back(static_cast<uint8_t>(level));

		// finally we copy in the log message itself
		payload.insert(payload.end(), logString.begin(),logString.end());

		return true;
	}

	// this function is not supported, but we need the callback in case we receive the command code
	// basically the retrieve log message command code is used to send out log messages from the motor
	// controller, so we do not require/accept the incoming command
	bool RealLogger::retrieveLogMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		return false;
	}

} // namespace Logging
