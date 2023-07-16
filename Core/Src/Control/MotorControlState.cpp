/*
 * MotorControlState.cpp
 *
 *  Created on: Jul 6, 2023
 *      Author: tim
 */

#include "MotorControlState.h"
#include "Logger.h"
#include <sstream>

extern Logging::Logger theLogger;

namespace MotorControl
{

	MotorControlState::MotorControlState(MotorDriver* p_motorDriver) :
			m_pMotorDriver{p_motorDriver}
	{
		// TODO Auto-generated constructor stub
		m_stateEntryTime = HAL_GetTick();
	}

	MotorControlState::~MotorControlState()
	{
		// TODO Auto-generated destructor stub
	}

	// return time, in system ticks, motor controller entered
	// current state
	uint32_t MotorControlState::timeInState() const
	{
		if (HAL_GetTick() > m_stateEntryTime)
		{
			return HAL_GetTick() - m_stateEntryTime;
		}
		else
		{
			// tick has rolled over
			return UINT32_MAX - m_stateEntryTime + HAL_GetTick() + 1;
		}
	}

	namespace MotorControlStates
	{
		// ***************************
		// ***** DISABLED STATE ******
		// ***************************

		Disabled::Disabled(MotorDriver* pMotorDriver) :
				MotorControlState(pMotorDriver)
		{
			m_pMotorDriver->wakeMotor();
			m_pMotorDriver->disableMotor();
			m_stateId = DISABLED;
			setName("DISABLED");
			theLogger.logString(info, "Motor controller entering DISABLED state; disabling motor driver");
		}

		MotorControlState* Disabled::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case DISABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return this;

				case ENABLE_MOTOR:
					// enter idle state, where motor driver will be enabled
					return new Idle(m_pMotorDriver);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return new Fault(m_pMotorDriver);

				case SLEEP:
					// enter sleep state, where motor will be disabled and driver put in sleep mode
					return new Sleeping(m_pMotorDriver);

				case MOTOR_COMMAND:
					// we are required to delete the command object if one is provided
					if (event.cmd)
						delete event.cmd;
					return this;

				default:
					theLogger.logString(error, "Unhandled event type in motor controller DISABLED state");
					return this;
			}
		}

		// ***************************
		// ******* IDLE STATE ********
		// ***************************

		Idle::Idle(MotorDriver* pMotorDriver) :
				MotorControlState(pMotorDriver)
		{
			m_pMotorDriver->enableMotor();
			m_stateId = IDLE;
			setName("IDLE");
			theLogger.logString(info, "Motor controller entering IDLE state, motor enabled");
		}

		MotorControlState* Idle::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case ENABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return this;

				case MOTOR_COMMAND:
					if (event.cmd)
						return new Ramp(m_pMotorDriver, event.cmd);
					else
					{
						theLogger.logString(error, "Motor command event received without valid command object");
						return this;
					}

				case DISABLE_MOTOR:
					// enter disabled state, where motor driver will be disabled
					return new Disabled(m_pMotorDriver);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return new Fault(m_pMotorDriver);

				case SLEEP:
					// enter sleeping state, where motor driver will be disabled and put to low power sleep state
					return new Sleeping(m_pMotorDriver);

				default:
					theLogger.logString(error, "Unhandled event type in motor controller IDLE state");
					return this;
			}
		}

		// ***************************
		// ****** FAULT STATE ********
		// ***************************

		Fault::Fault(MotorDriver* pMotorDriver) :
				MotorControlState(pMotorDriver)
		{
			m_pMotorDriver->disableMotor();
			m_stateId = FAULT;
			setName("FAULT");
			theLogger.logString(info, "Motor controller entering FAULT state, disabling motor driver");
		}

		MotorControlState* Fault::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case DISABLE_MOTOR:
				case ENABLE_MOTOR:
				case SLEEP:
				case WAKE:
				case MOTOR_FAULT:
					// do nothing
					return this;

				case CLEAR_FAULT:
					if (m_pMotorDriver->isDriverFaulted())
						return this;
					else
						return new Disabled(m_pMotorDriver);

				case MOTOR_COMMAND:
				// we are required to delete the command object if one is provided
				if (event.cmd)
					delete event.cmd;
				return this;

				default:
					theLogger.logString(error, "Unhandled event type in motor controller FAULT state");
					return this;
			}
		}

		// ***************************
		// ****** SLEEPING STATE ********
		// ***************************

		Sleeping::Sleeping(MotorDriver* pMotorDriver) :
				MotorControlState(pMotorDriver)
		{
			m_pMotorDriver->sleepMotor();
			m_stateId = SLEEPING;
			setName("SLEEPING");
			theLogger.logString(info, "Motor controller entering SLEEPING state, sleeping motor driver");
		}

		MotorControlState* Sleeping::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case DISABLE_MOTOR:
				case ENABLE_MOTOR:
				case SLEEP:
				case CLEAR_FAULT:
					// do nothing
					return this;

				case MOTOR_COMMAND:
					// we are required to delete the command object if one is provided
					if (event.cmd)
						delete event.cmd;
					return this;

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return new Fault(m_pMotorDriver);

				case WAKE:
					// TODO: handle this event
					theLogger.logString(info, "Received wake event, waking motor driver");
					return new Disabled(m_pMotorDriver);

				default:
					theLogger.logString(error, "Unhandled event type in motor controller SLEEPING state");
					return this;
			}
		}

		// ***************************
		// ******** RAMP STATE *******
		// ***************************

		// TODO: RESUME HERE - CHECK OUT STATE LOGIC FOR ALL STATES/EVENTS

		Ramp::Ramp(MotorDriver* pMotorDriver, MotorCommand* pMotorCommand) :
				MotorControlState(pMotorDriver),
				m_pMotorCommand{pMotorCommand}
		{
			m_stateId = RAMP;
			setName("RAMP");

			if (!m_pMotorCommand)
			{
				// no valid command received - exit state right away
				m_stateExitTime = m_stateEntryTime;

				// manufacture an artificial command so we can smoothly transition out of the state
				m_pMotorCommand = new MotorCommand{m_pMotorDriver->getForwardDirection(),	// forwardDirection
												   false,									// direcctionChange
												   m_pMotorDriver->getPwmDutyCycle(),		// pwmDutyCycle
												   0};										// rampTimeTicks

				theLogger.logString(error, "Entering ramp state, no motor command provided, speed will be unchanged");
			}
			else
			{
				// process motor command
				if (m_pMotorCommand->pwmDutyCycle == 0)
				{
					// we are ramping down to zero
					m_dutyCycleChangeRate = m_pMotorDriver->getPwmDutyCycle()/m_pMotorCommand->rampTimeTicks*-1;
					m_pMotorCommand->directionChange = false;
				}
				else
				{
					// state exit time is the ramp time plus the state entry time
					m_stateExitTime = m_stateEntryTime + m_pMotorCommand->rampTimeTicks;

					if (m_pMotorCommand->forwardDirection == m_pMotorDriver->getForwardDirection())
					{
						// ramping between speeds in same direction

						int32_t dutyCycleDelta = m_pMotorCommand->pwmDutyCycle - m_pMotorDriver->getPwmDutyCycle();
						m_dutyCycleChangeRate = dutyCycleDelta/m_pMotorCommand->rampTimeTicks;
						m_pMotorCommand->directionChange = false;
					}
					else
					{
						// ramping with direction change
						int32_t dutyCycleDelta = m_pMotorDriver->getPwmDutyCycle() + m_pMotorCommand->pwmDutyCycle;
						m_dutyCycleChangeRate = dutyCycleDelta/m_pMotorCommand->rampTimeTicks*-1;
						m_pMotorCommand->directionChange = true;
					}

					std::stringstream logStringStream;
					logStringStream << "Motor controller ramping from " << (m_pMotorDriver->getForwardDirection() ? "forward" : "reverse") <<
							":" << m_pMotorDriver->getPwmDutyCycle() << " to " << (m_pMotorCommand->forwardDirection ? "forward" : "reverse") <<
							":" << m_pMotorCommand->pwmDutyCycle;

					theLogger.logString(info, logStringStream.str());
				}
			}
		}

		Ramp::~Ramp()
		{
			if (m_pMotorCommand)
				delete m_pMotorCommand;
		}

		MotorControlState* Ramp::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case ENABLE_MOTOR:
				case SLEEP:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return this;

				case TICK:
					// check to see if it's time to exit the state
					// note we check if we are at least equal to the state exit time, but not more than 1000 ticks greater; this
					// allows us to tolerate a rollover in the tick timer without having to worry if we miss the exact tick when
					// we should exit
					if (HAL_GetTick() >= m_stateExitTime && HAL_GetTick() < (m_stateExitTime + 1000))
					{
						// ensure we've got the correct final duty cycle
						m_pMotorDriver->setPwmDutyCycle(m_pMotorCommand->pwmDutyCycle);

						// exit to the correct state
						if (m_pMotorCommand->pwmDutyCycle == 0)
							return new Idle(m_pMotorDriver);
						else
							return new Running(m_pMotorDriver);
					}

					{
						// new scope block here so that this local variable is not visible outside of TICK handler case
						int32_t newDutyCycle = m_pMotorDriver->getPwmDutyCycle() + m_dutyCycleChangeRate;

						if (newDutyCycle < 0)
						{
							if (m_pMotorCommand->directionChange)
							{
								// zero crossing in our PWM ramp, we need to change motor direction and
								// continue ramping to final duty cycle
								newDutyCycle *= -1;
								m_pMotorDriver->setForwardDirection(m_pMotorCommand->forwardDirection);

								// we will change our delta value, once we reach zero duty cycle we will start to increase
								// from zero again in the opposite directtion
								m_dutyCycleChangeRate *= -1;

								m_pMotorDriver->setPwmDutyCycle(newDutyCycle);
							}
							else
							{
								// this is an unexpected zero crossing, we will just zero the duty cycle and log the incident
								m_pMotorDriver->setPwmDutyCycle(0);
								theLogger.logString(warning, "Unexpected zero crossing in motor driver PWM ramp, idling motor");
								return new Idle(m_pMotorDriver);
							}
						}
						else
						{
							m_pMotorDriver->setPwmDutyCycle(newDutyCycle);
						}
					}

					return this;

				case MOTOR_COMMAND:
					// We don't process new commands while ramping
					theLogger.logString(warning, "Ignoring new motor command received while ramping");

					// we are required to delete the command object if one is provided
					if (event.cmd)
						delete event.cmd;

					return this;

				case DISABLE_MOTOR:
					return new Disabled(m_pMotorDriver);

				case MOTOR_FAULT:
					return new Fault(m_pMotorDriver);

				default:
					theLogger.logString(error, "Unhandled event type in motor controller RAMP state");
					return this;
			}
		}

		// ***************************
		// ****** RUNNING STATE ******
		// ***************************

		Running::Running(MotorDriver* pMotorDriver) :
				MotorControlState(pMotorDriver)
		{
			std::stringstream logStringStream;
			m_stateId = RUNNING;
			setName("RUNNING");

			logStringStream << "Motor controller entering RUNNING state in " << (m_pMotorDriver->getForwardDirection() ? "forward" : "reverse")
					<< " direction at duty cycle " << m_pMotorDriver->getPwmDutyCycle();
			theLogger.logString(info, logStringStream.str());
		}

		MotorControlState* Running::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case ENABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return this;

				case MOTOR_COMMAND:
					if (event.cmd)
						return new Ramp(m_pMotorDriver, event.cmd);
					else
					{
						theLogger.logString(error, "Motor command event received without valid command object");
						return this;
					}

				case DISABLE_MOTOR:
					// enter disabled state, where motor driver will be disabled
					return new Disabled(m_pMotorDriver);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return new Fault(m_pMotorDriver);

				case SLEEP:
					// enter sleeping state, where motor driver will be disabled and put to low power sleep state
					return new Sleeping(m_pMotorDriver);

				default:
					theLogger.logString(error, "Unhandled event type in motor controller IDLE state");
					return this;
			}
		}
	} // namespace MotorControlStates
} // namespace MotorControl
