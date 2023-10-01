/*
 * MotorCommandHandler.cpp
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#include "MotorCommandHandler.h"
#include "SingletonFactory.h"
#include <cstring>
#include <algorithm>

namespace MotorControl {
	MotorCommandHandler::MotorCommandHandler(std::shared_ptr<Controller>& pController) :
			m_pController{pController}
	{
		SingletonFactory factory;
		m_pLogger = factory.retrieve<Logging::ILogger>();
	}

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
			m_pLogger->logString(error, "Invalid payload size in ramp motor on command");
			return false;
		}

		uint16_t driveLevel{0};
		std::memcpy(&driveLevel, &msg.getPayload().data()[0], 2);

		if (driveLevel > 1000)
		{
			m_pLogger->logString(error, "Invalid drive level size in ramp motor on command: " + std::to_string(driveLevel));
			return false;
		}

		if (msg.getPayload().data()[2] != 1 && msg.getPayload().data()[2] != 0)
		{
			m_pLogger->logString(error, "Invalid direction in ramp motor on command: " + std::to_string(msg.getPayload().data()[2]));
			return false;
		}

		uint16_t rampTime_ms{0};
		std::memcpy(&rampTime_ms, &msg.getPayload().data()[3], 2);

		if (rampTime_ms < 25)
		{
			m_pLogger->logString(error, "Invalid ramp time in ramp motor on command: " + std::to_string(rampTime_ms));
			return false;
		}

		// Check that change rate is at least one tenth of a percent per ms/tick and fail command if not
		uint16_t targetDutyCycle{0};
		std::memcpy(&targetDutyCycle, &msg.getPayload().data()[0], 2);

		int32_t dutyCycleChange = std::abs(static_cast<int32_t>(m_pController->getDutyCycle()) -
								  static_cast<int32_t>(std::clamp(targetDutyCycle,
										  	  	  	  	  	      static_cast<uint16_t>(0),
																  m_pController->getMaxDutyCycle())));

		if (dutyCycleChange < rampTime_ms)
		{
			m_pLogger->logString(error, "Duty cycle change rate is too low for ramp command");
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
			m_pLogger->logString(error, "Invalid payload size in ramp motor off command");
			return false;
		}

		uint16_t rampTime_ms{0};
		std::memcpy(&rampTime_ms, msg.getPayload().data(), 2);

		if (rampTime_ms < 25)
		{
			m_pLogger->logString(error, "Invalid ramp time in ramp motor off command: " + std::to_string(rampTime_ms));
			return false;
		}

		// TODO: check that change rate is at least one tenth of a percent per ms/tick and fail command if not

		m_pController->sendCommand(0, 0, rampTime_ms);
		return true;
	}

	bool MotorCommandHandler::setMaxDutyCycle(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// the set max duty cycle command should have a 2 byte payload for the max duty cycle as tenths of a percent
		// of the duty cycle limit
		if (msg.getPayload().size() != 2)
		{
			m_pLogger->logString(error, "Invalid payload size in set max duty cycle command");
			return false;
		}

		uint16_t maxDutyCycle_pct{0};
		std::memcpy(&maxDutyCycle_pct, msg.getPayload().data(), 2);

		if (maxDutyCycle_pct > 1000)
		{
			m_pLogger->logString(error, "Invalid duty cycle in set max duty cycle command: " + std::to_string(maxDutyCycle_pct));
			return false;
		}

		m_pController->setMaxDutyCycle(maxDutyCycle_pct);
		return true;
	}

	bool MotorCommandHandler::injectFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->injectFault();
		return true;
	}

	bool MotorCommandHandler::clearFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		m_pController->clearFault();
		return true;
	}

	bool MotorCommandHandler::getStatus(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
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
	}

} /* namespace MotorControl */
