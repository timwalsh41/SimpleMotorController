/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include "MessageHandler.h"
#include "CommandInterface.h"
#include "OutputGateway.h"

extern OutputGateway theOutputGateway;

MessageHandler::MessageHandler()
{

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

			theOutputGateway.logString(error, std::string("Received message with bad CRC"));
			continue;
		}

		// TODO: Resume here
		theOutputGateway.logString(debug, "Received valid message, processing");
	}
}
