/*
 * MotorCommandHandler.cpp
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#include "MotorCommandHandler.h"

namespace MotorControl {

	bool MotorCommandHandler::enableMotor(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		m_pController->enableMotor();
		return true;
	}

	bool MotorCommandHandler::disableMotor(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		m_pController->disableMotor();
		return true;
	}

	bool MotorCommandHandler::sleepDriver(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		m_pController->sleepDriver();
		return true;
	}

	bool MotorCommandHandler::wakeDriver(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		m_pController->wakeDriver();
		return true;
	}

	bool MotorCommandHandler::getMotorPosition(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		int32_t* ptr;
		payload = new uint8_t[4];
		payloadLength = 4;

		// get position from motor driver
		int32_t position = m_pController->getMotorPosition();

		// now copy it over into the payload byte array
		ptr = (int32_t*) payload;
		*ptr = position;

		return true;
	}

	bool MotorCommandHandler::getMotorSpeed(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength)
	{
		float* ptr;
		payload = new uint8_t[4];
		payloadLength = 4;

		// get position from motor driver
		float speed = m_pController->getMotorSpeed();

		// now copy it over into the payload byte array
		ptr = (float*) payload;
		*ptr = speed;

		return true;
	}

} /* namespace MotorControl */
