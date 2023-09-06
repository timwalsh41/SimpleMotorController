/*
 * MotorCommandHandler.cpp
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#include "MotorCommandHandler.h"
#include "Logger.h"
#include <cstring>

extern Logging::Logger theLogger;

namespace MotorControl {

	bool MotorCommandHandler::enableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->enableMotor();
		return true;
	}

	bool MotorCommandHandler::disableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->disableMotor();
		return true;
	}

	bool MotorCommandHandler::sleepDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->sleepDriver();
		return true;
	}

	bool MotorCommandHandler::wakeDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->wakeDriver();
		return true;
	}

	bool MotorCommandHandler::getMotorPosition(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// get position from motor driver
		int32_t position = m_pController->getMotorPosition();

		// now copy it over into the payload byte array
		// TODO: verify this works, and is memory safe
		payload.reserve(4);
		std::memcpy(payload.data(), &position, 4);

		return true;
	}

	bool MotorCommandHandler::getMotorSpeed(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// get position from motor driver
		float speed = m_pController->getMotorSpeed();

		// now copy it over into the payload byte array
		// TODO: verify this works, and is memory safe
		payload.reserve(4);
		std::memcpy(payload.data(), &speed, 4);

		return true;
	}

	bool MotorCommandHandler::getMotorDirection(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// get position from motor driver
		bool direction = m_pController->getMotorDirection();
		payload.push_back(direction ? 1 : 0);

		return true;
	}

	bool MotorCommandHandler::rampMotorOn(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// the ramp motor on command should have a 5 byte payload (2 byte drive level (in tenths of a percent of full drive), one
		// byte direction, and two byte ramp time (in ms)
		if (msg.getPayload().size() != 5)
		{
			theLogger.logString(error, "Invalid payload size in ramp motor on command");
			return false;
		}

		uint16_t driveLevel{0};
		std::memcpy(&driveLevel, &msg.getPayload().data()[0], 2);

		if (driveLevel > 1000)
		{
			theLogger.logString(error, "Invalid drive level size in ramp motor on command: " + std::to_string(driveLevel));
			return false;
		}

		if (msg.getPayload().data()[2] != 1 && msg.getPayload().data()[2] != 0)
		{
			theLogger.logString(error, "Invalid direction in ramp motor on command: " + std::to_string(msg.getPayload().data()[2]));
			return false;
		}

		uint16_t rampTime_ms{0};
		std::memcpy(&rampTime_ms, &msg.getPayload().data()[3], 2);

		if (rampTime_ms < 25)
		{
			theLogger.logString(error, "Invalid ramp time in ramp motor on command: " + std::to_string(rampTime_ms));
			return false;
		}

		// convert drive level from tenths of a percent to PWM duty cycle
		uint32_t dutyCycle = static_cast<uint32_t>(driveLevel)*static_cast<uint32_t>(m_pController->getMaxDutyCycle())/1000;

		m_pController->sendCommand(msg.getPayload().data()[2], static_cast<uint16_t>(dutyCycle), rampTime_ms);
		return true;
	}

	bool MotorCommandHandler::rampMotorOff(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// the ramp motor off command should have a 2 byte payload for the ramp time in ms
		if (msg.getPayload().size() != 2)
		{
			theLogger.logString(error, "Invalid payload size in ramp motor off command");
			return false;
		}

		uint16_t rampTime_ms{0};
		std::memcpy(&rampTime_ms, msg.getPayload().data(), 2);

		if (rampTime_ms < 25)
		{
			theLogger.logString(error, "Invalid ramp time in ramp motor off command: " + std::to_string(rampTime_ms));
			return false;
		}

		m_pController->sendCommand(0, 0, rampTime_ms);
		return true;
	}

	bool MotorCommandHandler::setMaxDutyCycle(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// the set max duty cycle command should have a 2 byte payload for the max duty cycle as tenths of a percent
		// of the duty cycle limit
		if (msg.getPayload().size() != 2)
		{
			theLogger.logString(error, "Invalid payload size in set max duty cycle command");
			return false;
		}

		uint16_t maxDutyCycle_pct{0};
		std::memcpy(&maxDutyCycle_pct, msg.getPayload().data(), 2);

		if (maxDutyCycle_pct > 1000)
		{
			theLogger.logString(error, "Invalid duty cycle in set max duty cycle command: " + std::to_string(maxDutyCycle_pct));
			return false;
		}

		m_pController->setMaxDutyCycle(maxDutyCycle_pct);
		return true;
	}

	bool MotorCommandHandler::injectFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->motorFault();
		return true;
	}

} /* namespace MotorControl */
