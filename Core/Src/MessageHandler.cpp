/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include "MessageHandler.h"
#include "CommandInterface.h"
#include "OutputGateway.h"
#include "Controller.h"
#include "MotorCommandHandler.h"
#include "CurrentSensor.h"
#include "Logger.h"
#include "Version.h"

extern MotorControl::Controller theController;
extern MotorControl::MotorCommandHandler theMotorCommandHandler;
extern MessageHandling::OutputGateway theOutputGateway;
extern Sensors::CurrentSensor theCurrentSensor;
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
		// Grab the latest message, we'll just move it out of the queue now so we don't need to keep track of
		// when we can release it
		msg = std::move(m_messageQueue.front());
		m_messageQueue.pop();

		if (!msg.checkCRC())
		{
			// invalid message CRC
			uint8_t payload = ACK_INVALID_CRC;
			msg.setPayload(payload);

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
				msg.setPayload(payload);

				theLogger.logString(error, std::string("Received message with invalid command code"));
			}
			else
			{
				// valid command code, attempt to process command
				std::vector<uint8_t> returnData{};

				if (m_commandMap[msg.getCommandCode()](msg, returnData))
				{
					std::vector<uint8_t> payload;
					payload.push_back(ACK_SUCCESSFUL);

					// successfully processed command, we will log this in the command function itself
					if (!returnData.empty())
					{
						// not all messages have a payload, so if we receive one then copy it into the output message
						payload.insert(payload.end(), returnData.begin(), returnData.end());
					}

					msg.setPayload(payload);
				}
				else
				{
					// failure processing command
					uint8_t payload = ACK_INVALID_PAYLOAD;
					msg.setPayload(payload);

					theLogger.logString(error, std::string("Unable to execute command"));
				}
			}
		}

		// msg has the original command code, we just need to set the ack bit and calculate the CRC
		msg.setAck();
		msg.setCRC();
		theOutputGateway.sendMessage(msg);
	}
}

void MessageHandler::buildCommandMap()
{
	m_commandMap[CMD_ENABLE_MOTOR] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.enableMotor(msg, payload);
			};
	m_commandMap[CMD_DISABLE_MOTOR] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.disableMotor(msg, payload);
			};
	m_commandMap[CMD_SLEEP_DRIVER] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.sleepDriver(msg, payload);
			};
	m_commandMap[CMD_WAKE_DRIVER] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.wakeDriver(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_POS] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.getMotorPosition(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_SPEED] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.getMotorSpeed(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_DIR] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.getMotorDirection(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_CURRENT] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theCurrentSensor.getMotorCurrent(msg, payload);
			};
	m_commandMap[CMD_RAMP_MOTOR_ON] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.rampMotorOn(msg, payload);
			};
	m_commandMap[CMD_RAMP_MOTOR_OFF] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.rampMotorOff(msg, payload);
			};
	m_commandMap[CMD_SET_MAX_DC] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.setMaxDutyCycle(msg, payload);
			};
	m_commandMap[CMD_INJECT_FAULT] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				return theMotorCommandHandler.injectFault(msg, payload);
			};
	m_commandMap[CMD_GET_VERSION] = Utility::sendVersionMessage;
	m_commandMap[CMD_GET_STATUS] = [](const Message& msg, std::vector<uint8_t>& payload)
			{
				// return payload is 8 bytes
				payload.reserve(8);

				// byte 0: motor controller state
				payload.push_back(static_cast<uint8_t>(theController.getState()));

				// byte 1: bitmapped status byte
				uint8_t statusByte{0};
				statusByte |= (theController.getMotorDirection() ? 1 : 0) << 0;	// bit 0: motor direction
				statusByte |= (theController.isDriverFaulted() ? 1 : 0) << 1;	// bit 1: driver fault status
				statusByte |= (theController.isDriverSleeping() ? 1 : 0) << 2;	// bit 2: driver sleep status
				statusByte |= (theLogger.logMessagesPending() ? 1 : 0) << 3;	// bit 3: log messages pending
				payload.push_back(statusByte);

				// bytes 2-3: motor drive level (tenths of a percent)
				// TODO: verify endianness
				uint16_t driveLevel = theController.getMotorPwmLevel();
				payload.push_back(static_cast<uint8_t>((driveLevel >> 0) & 0xFF));
				payload.push_back(static_cast<uint8_t>((driveLevel >> 8) & 0xFF));

				// bytes 4-7: time in current state (ms)
				// TODO: verify endianness
				uint32_t timeInState = theController.getTimeInState();
				payload.push_back(static_cast<uint8_t>((timeInState >> 0) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 8) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 16) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 24) & 0xFF));

				return true;
			};
}

} /* namespace MessageHandling */
