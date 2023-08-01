/*
 * MotorCommandHandler.cpp
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#include "MotorCommandHandler.h"
#include <cstring>

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

} /* namespace MotorControl */
