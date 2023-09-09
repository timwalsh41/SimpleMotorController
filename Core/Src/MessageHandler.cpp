/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include "MessageHandler.h"
#include "CommandInterface.h"
#include "Version.h"
#include "SingletonFactory.h"

namespace MessageHandling
{

MessageHandler::MessageHandler()
{
	SingletonFactory factory;
	m_pLogger = factory.retrieve<Logging::ILogger>();
	m_pController = factory.retrieve<MotorControl::Controller>();
	m_pMotorCommandHandler = factory.retrieve<MotorControl::MotorCommandHandler>();
	m_pOutputGateway = factory.retrieve<MessageHandling::OutputGateway>();
	m_pCurrentSensor = factory.retrieve<Sensors::CurrentSensor>();

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

			m_pLogger->logString(error, std::string("Received message with bad CRC"));
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

				m_pLogger->logString(error, std::string("Received message with invalid command code"));
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

					m_pLogger->logString(error, std::string("Unable to execute command"));
				}
			}
		}

		// msg has the original command code, we just need to set the ack bit and calculate the CRC
		msg.setAck();
		msg.setCRC();
		m_pOutputGateway->sendMessage(msg);
	}
}

void MessageHandler::buildCommandMap()
{
	m_commandMap[CMD_ENABLE_MOTOR] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->enableMotor(msg, payload);
			};
	m_commandMap[CMD_DISABLE_MOTOR] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->disableMotor(msg, payload);
			};
	m_commandMap[CMD_SLEEP_DRIVER] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->sleepDriver(msg, payload);
			};
	m_commandMap[CMD_WAKE_DRIVER] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->wakeDriver(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_POS] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->getMotorPosition(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_SPEED] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->getMotorSpeed(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_DIR] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->getMotorDirection(msg, payload);
			};
	m_commandMap[CMD_GET_MOTOR_CURRENT] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pCurrentSensor->getMotorCurrent(msg, payload);
			};
	m_commandMap[CMD_RAMP_MOTOR_ON] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->rampMotorOn(msg, payload);
			};
	m_commandMap[CMD_RAMP_MOTOR_OFF] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->rampMotorOff(msg, payload);
			};
	m_commandMap[CMD_SET_MAX_DC] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->setMaxDutyCycle(msg, payload);
			};
	m_commandMap[CMD_INJECT_FAULT] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pMotorCommandHandler->injectFault(msg, payload);
			};
	m_commandMap[CMD_GET_VERSION] = Utility::sendVersionMessage;
	m_commandMap[CMD_GET_STATUS] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				// return payload is 8 bytes
				payload.reserve(8);

				// byte 0: motor controller state
				payload.push_back(static_cast<uint8_t>(m_pController->getState()));

				// byte 1: bitmapped status byte
				uint8_t statusByte{0};
				statusByte |= (m_pController->getMotorDirection() ? 1 : 0) << 0;	// bit 0: motor direction
				statusByte |= (m_pController->isDriverFaulted() ? 1 : 0) << 1;	// bit 1: driver fault status
				statusByte |= (m_pController->isDriverSleeping() ? 1 : 0) << 2;	// bit 2: driver sleep status
				statusByte |= (m_pLogger->logMessagesPending() ? 1 : 0) << 3;	// bit 3: log messages pending
				payload.push_back(statusByte);

				// bytes 2-3: motor drive level (tenths of a percent)
				// TODO: verify endianness
				uint16_t driveLevel = m_pController->getMotorPwmLevel();
				payload.push_back(static_cast<uint8_t>((driveLevel >> 0) & 0xFF));
				payload.push_back(static_cast<uint8_t>((driveLevel >> 8) & 0xFF));

				// bytes 4-7: time in current state (ms)
				// TODO: verify endianness
				uint32_t timeInState = m_pController->getTimeInState();
				payload.push_back(static_cast<uint8_t>((timeInState >> 0) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 8) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 16) & 0xFF));
				payload.push_back(static_cast<uint8_t>((timeInState >> 24) & 0xFF));

				return true;
			};
	m_commandMap[CMD_GET_LOG_MESSAGE] = [this](const Message& msg, std::vector<uint8_t>& payload)
			{
				return m_pLogger->retrieveLogMessage(msg, payload);
			};
}

} /* namespace MessageHandling */
