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
		// TODO Auto-generated constructor stub
		m_state = new MotorControlStates::Disabled(&m_motorDriver);
		std::memset(m_pMotorSpeedBuffer, 0, sizeof(float)*VELOCITY_BUFFER_SIZE);
	}

	Controller::~Controller() {
		// TODO Auto-generated destructor stub
	}

	void Controller::tick()
	{
		MotorControlState* nextState{nullptr};

		if (m_state->getId() == FAULT)
			nextState = m_state->handleEvent(MotorEvent{CLEAR_FAULT, nullptr});
		else if (m_motorDriver.isDriverFaulted())
			// TODO: Could move fault detection to interrupt
			nextState = m_state->handleEvent(MotorEvent{MOTOR_FAULT, nullptr});
		else
			nextState = m_state->handleEvent(MotorEvent{TICK, nullptr});

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

	void Controller::updateState(MotorControlState* nextState)
	{
		if (!nextState)
		{
			theLogger.logString(error, "Invalid state returned from state " + m_state->getName() +
					" event handler, remaining in state" + m_state->getName());
		}
		else if (nextState != m_state)
		{
			delete m_state;
			m_state = nextState;
		}
	}

	// Motor fault detected, e.g. by GPIO interrupt
	void Controller::motorFault()
	{
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{MOTOR_FAULT, nullptr});
		updateState(nextState);
	}

	void Controller::enableMotor()
	{
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{ENABLE_MOTOR, nullptr});
		updateState(nextState);
	}

	void Controller::disableMotor()
	{
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{DISABLE_MOTOR, nullptr});
		updateState(nextState);
	}

	void Controller::sleepMotor()
	{
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{SLEEP, nullptr});
		updateState(nextState);
	}

	void Controller::wakeMotor()
	{
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{WAKE, nullptr});
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
		MotorCommand* cmd = new MotorCommand{forwardDirection, false, dutyCycle, rampTimeMs};
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{MOTOR_COMMAND, cmd});
		updateState(nextState);
	}

	void Controller::idleMotor()
	{
		// TODO: this assumes tick time is 1 ms, otherwise a conversion is needed
		MotorCommand* cmd = new MotorCommand{true, false, 0, DEFAULT_RAMP_TIME_MS};
		MotorControlState* nextState = m_state->handleEvent(MotorEvent{MOTOR_COMMAND, cmd});
		updateState(nextState);
	}


} /* namespace MotorControl */
