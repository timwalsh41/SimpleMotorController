/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include "MessageHandler.h"
#include "CommandInterface.h"
#include "OutputGateway.h"
#include "Logger.h"
#include "Version.h"

extern MessageHandling::OutputGateway theOutputGateway;
extern Logging::Logger theLogger;

namespace MessageHandling
{

MessageHandler::MessageHandler()
{
	buildCommandMap();
}

MessageHandler::~MessageHandler()
{

}

void MessageHandler::loadIncomingMessage(Message& msg)
{
	m_messageQueue.push(std::move(msg));
}

void MessageHandler::processMessages()
{
	Message msg;

	while (!m_messageQueue.empty())
	{
		msg = std::move(m_messageQueue.front());
		m_messageQueue.pop();

		if (!msg.checkCRC())
		{
			Message response;
			uint8_t payload;

			response.setCommandCode(msg.getCommandCode());
			response.setAck();

			payload = ACK_INVALID_CRC;
			response.setPayload(&payload, 1);
			response.setCRC();

			theOutputGateway.sendMessage(response);

			theLogger.logString(error, std::string("Received message with bad CRC"));
			continue;
		}

		auto key = m_commandMap.find(msg.getCommandCode());
		if (key == m_commandMap.end())
		{
			// invalid command code recceived
			Message response;
			uint8_t payload;

			response.setCommandCode(msg.getCommandCode());
			response.setAck();

			payload = ACK_INVALID_COMMAND;
			response.setPayload(&payload, 1);
			response.setCRC();

			theOutputGateway.sendMessage(response);

			theLogger.logString(error, std::string("Received message with invalid command code"));
			continue;
		}

		if (!m_commandMap[msg.getCommandCode()](msg))
		{
			theLogger.logString(error, std::string("Unable to execute command"));
		}
	}
}

void MessageHandler::buildCommandMap()
{
	m_commandMap[CMD_GET_VERSION] = Utility::sendVersionMessage;
}

} /* namespace MessageHandling */
