/*
 * Controller.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#include "Controller.h"
#include "Logger.h"
#include <cstring>

extern Logging::Logger theLogger;

namespace MotorControl
{
	Controller::Controller() :
		m_lastEncoderCount(m_motorDriver.getEncoderCounts())
	{
		m_state = std::make_unique<MotorControlStates::Disabled>(&m_motorDriver);
		std::memset(m_pMotorSpeedBuffer, 0, sizeof(float)*VELOCITY_BUFFER_SIZE);
	}

	void Controller::tick()
	{
		std::unique_ptr<MotorControlState> nextState;

		// create a null motor command object that we use to call the motor state event handler
		// for events without a command
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);

		if (m_state->getId() == FAULT)
			nextState = m_state->handleEvent(MotorEvent{CLEAR_FAULT, nullMotorCommand});
		else if (m_motorDriver.isDriverFaulted())
			// TODO: Could move fault detection to interrupt
			nextState = m_state->handleEvent(MotorEvent{MOTOR_FAULT, nullMotorCommand});
		else
			nextState = m_state->handleEvent(MotorEvent{TICK, nullMotorCommand});

		updateState(nextState);
		updateSpeed();
		m_motorDriver.update();
	}

	void Controller::updateSpeed()
	{
		int32_t currentEncoderCount = m_motorDriver.getEncoderCounts();
		m_instantaneousMotorSpeed = static_cast<float>(currentEncoderCount - m_lastEncoderCount);
		m_bufferedMotorSpeed += (static_cast<float>(m_instantaneousMotorSpeed) -
								 m_pMotorSpeedBuffer[m_speedBufferIdx]) / VELOCITY_BUFFER_SIZE;
		m_pMotorSpeedBuffer[m_speedBufferIdx++] = m_instantaneousMotorSpeed;
		if (m_speedBufferIdx == VELOCITY_BUFFER_SIZE)
			m_speedBufferIdx = 0;

	}

	void Controller::updateState(std::unique_ptr<MotorControlState>& nextState)
	{
		if (nextState.get())
		{
			// a new state was returned
			m_state = std::move(nextState);
		}
	}

	// Motor fault detected, e.g. by GPIO interrupt
	void Controller::motorFault()
	{
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{MOTOR_FAULT, nullMotorCommand});
		updateState(nextState);
	}

	void Controller::enableMotor()
	{
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{ENABLE_MOTOR, nullMotorCommand});
		updateState(nextState);
	}

	void Controller::disableMotor()
	{
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{DISABLE_MOTOR, nullMotorCommand});
		updateState(nextState);
	}

	void Controller::sleepDriver()
	{
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{SLEEP, nullMotorCommand});
		updateState(nextState);
	}

	void Controller::wakeDriver()
	{
		auto nullMotorCommand = static_cast<std::unique_ptr<MotorCommand>>(nullptr);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{WAKE, nullMotorCommand});
		updateState(nextState);
	}

	bool Controller::getMotorDirection() const
	{
		return m_motorDriver.getForwardDirection();
	}

	int32_t Controller::getMotorPosition() const
	{
		return m_motorDriver.getEncoderCounts();
	}

	uint16_t Controller::getMotorPwmLevel() const
	{
		return static_cast<uint16_t>(m_motorDriver.getPwmDutyCycle());
	}

	void Controller::sendCommand(bool forwardDirection, uint16_t dutyCycle, uint16_t rampTimeMs)
	{
		// TODO: this assumes tick time is 1 ms, otherwise a conversion is needed
		std::unique_ptr<MotorCommand> cmd = std::make_unique<MotorCommand>(forwardDirection, false, dutyCycle, rampTimeMs);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{MOTOR_COMMAND, cmd});
		updateState(nextState);
	}

	void Controller::idleMotor()
	{
		// TODO: this assumes tick time is 1 ms, otherwise a conversion is needed
		std::unique_ptr<MotorCommand> cmd = std::make_unique<MotorCommand>(true, false, 0, DEFAULT_RAMP_TIME_MS);
		std::unique_ptr<MotorControlState> nextState = m_state->handleEvent(MotorEvent{MOTOR_COMMAND, cmd});
		updateState(nextState);
	}


} /* namespace MotorControl */
