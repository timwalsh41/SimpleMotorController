/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include "MessageHandler.h"
#include "CommandInterface.h"
#include "OutputGateway.h"
#include "MotorCommandHandler.h"
#include "Logger.h"
#include "Version.h"

extern MotorControl::MotorCommandHandler theMotorCommandHandler;
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
	msg.setCommandCode(msg.getCommandCode());
	msg.setAck();

	while (!m_messageQueue.empty())
	{
		msg = std::move(m_messageQueue.front());
		m_messageQueue.pop();

		if (!msg.checkCRC())
		{
			// invalid message CRC
			uint8_t payload = ACK_INVALID_CRC;
			msg.setPayload(&payload, 1);

			theLogger.logString(error, std::string("Received message with bad CRC"));
		}
		else
		{
			// valid CRC, now check command code
			auto key = m_commandMap.find(msg.getCommandCode());
			if (key == m_commandMap.end())
			{
				// invalid command code received
				uint8_t payload = ACK_INVALID_COMMAND;
				msg.setPayload(&payload, 1);

				theLogger.logString(error, std::string("Received message with invalid command code"));
			}
			else
			{
				// valid command code, attempt to process command
				uint8_t* returnData{nullptr};
				uint8_t returnDataLength{0};

				if (m_commandMap[msg.getCommandCode()](msg, returnData, returnDataLength))
				{
					// successfully processed command, we will log this in the command function itself
					if (returnData)
					{
						// not all messages have a payload, so if we receive one then copy it into the output message
						msg.setPayload(returnData, returnDataLength);
						delete[] returnData;
					}
				}
				else
				{
					// failure processing command
					uint8_t payload = ACK_INVALID_PAYLOAD;
					msg.setPayload(&payload, 1);

					if (returnData)
						delete[] returnData;

					theLogger.logString(error, std::string("Unable to execute command"));
				}
			}
		}

		msg.setCRC();
		theOutputGateway.sendMessage(msg);
	}
}

// TODO: I INSERTED CMD_GET_MOTOR_SPEED AT 0x42 ON PLANE WITHOUT INTERNET, NEED TO UPDATE COMMAND MAP ON ONENOTE

void MessageHandler::buildCommandMap()
{
	m_commandMap[CMD_ENABLE_MOTOR] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.enableMotor(msg, payload, payloadLength);
			};
	m_commandMap[CMD_DISABLE_MOTOR] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.disableMotor(msg, payload, payloadLength);
			};
	m_commandMap[CMD_SLEEP_DRIVER] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.sleepDriver(msg, payload, payloadLength);
			};
	m_commandMap[CMD_WAKE_DRIVER] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.wakeDriver(msg, payload, payloadLength);
			};
	m_commandMap[CMD_GET_MOTOR_POS] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.getMotorPosition(msg, payload, payloadLength);
			};
	m_commandMap[CMD_GET_MOTOR_SPEED] = [](const Message& msg, uint8_t* payload, uint8_t& payloadLength)
			{
				return theMotorCommandHandler.getMotorSpeed(msg, payload, payloadLength);
			};

	m_commandMap[CMD_GET_VERSION] = Utility::sendVersionMessage;
}

} /* namespace MessageHandling */
