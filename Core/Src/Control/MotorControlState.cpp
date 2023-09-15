/*
 * MotorControlState.cpp
 *
 *  Created on: Jul 6, 2023
 *      Author: tim
 */

#include "MotorControlState.h"
#include "SingletonFactory.h"
#include <sstream>

namespace MotorControl
{

	MotorControlState::MotorControlState(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
			m_pMotorDriver{pMotorDriver},
			m_pLogger{pLogger}
	{
		SingletonFactory factory;
		m_pTimebase = factory.retrieve<ITimebase>();
		m_stateEntryTime = m_pTimebase->getTime_ms();
	}

	MotorControlState::~MotorControlState()
	{

	}

	// return time, in system ticks, motor controller entered
	// current state
	uint32_t MotorControlState::timeInState() const
	{
		if (m_pTimebase->getTime_ms() > m_stateEntryTime)
		{
			return m_pTimebase->getTime_ms() - m_stateEntryTime;
		}
		else
		{
			// tick has rolled over
			return UINT32_MAX - m_stateEntryTime + m_pTimebase->getTime_ms() + 1;
		}
	}

	namespace MotorControlStates
	{
		// ***************************
		// ***** DISABLED STATE ******
		// ***************************

		Disabled::Disabled(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
				MotorControlState(pMotorDriver, pLogger)
		{
			m_pMotorDriver->wakeMotor();
			m_pMotorDriver->disableMotor();
			m_stateId = DISABLED;
			setName("DISABLED");
			m_pLogger->logString(info, "Motor controller entering DISABLED state; disabling motor driver");
		}

		std::unique_ptr<MotorControlState> Disabled::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case DISABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
				case MOTOR_COMMAND:
					// do nothing
					return nullptr;

				case ENABLE_MOTOR:
					// enter idle state, where motor driver will be enabled
					return std::make_unique<Idle>(m_pMotorDriver, m_pLogger);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return std::make_unique<Fault>(m_pMotorDriver, m_pLogger);

				case SLEEP:
					// enter sleep state, where motor will be disabled and driver put in sleep mode
					return std::make_unique<Sleeping>(m_pMotorDriver, m_pLogger);

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller DISABLED state");
					return nullptr;
			}
		}

		// ***************************
		// ******* IDLE STATE ********
		// ***************************

		Idle::Idle(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
				MotorControlState(pMotorDriver, pLogger)
		{
			m_pMotorDriver->enableMotor();
			m_stateId = IDLE;
			setName("IDLE");
			m_pLogger->logString(info, "Motor controller entering IDLE state, motor enabled");
		}

		std::unique_ptr<MotorControlState> Idle::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case ENABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return nullptr;

				case MOTOR_COMMAND:
					if (event.cmd)
						return std::make_unique<Ramp>(m_pMotorDriver, m_pLogger, event.cmd);
					else
					{
						m_pLogger->logString(error, "Motor command event received without valid command object");
						return nullptr;
					}

				case DISABLE_MOTOR:
					// enter disabled state, where motor driver will be disabled
					return std::make_unique<Disabled>(m_pMotorDriver, m_pLogger);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return std::make_unique<Fault>(m_pMotorDriver, m_pLogger);

				case SLEEP:
					// enter sleeping state, where motor driver will be disabled and put to low power sleep state
					return std::make_unique<Sleeping>(m_pMotorDriver, m_pLogger);

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller IDLE state");
					return nullptr;
			}
		}

		// ***************************
		// ****** FAULT STATE ********
		// ***************************

		Fault::Fault(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
				MotorControlState(pMotorDriver, pLogger)
		{
			m_pMotorDriver->disableMotor();
			m_stateId = FAULT;
			setName("FAULT");
			m_pLogger->logString(info, "Motor controller entering FAULT state, disabling motor driver");
		}

		std::unique_ptr<MotorControlState> Fault::handleEvent(MotorEvent event)
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
					return nullptr;

				case CLEAR_FAULT:
					if (m_pMotorDriver->isDriverFaulted())
						return nullptr;
					else
						return std::make_unique<Disabled>(m_pMotorDriver, m_pLogger);

				case MOTOR_COMMAND:
					return nullptr;

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller FAULT state");
					return nullptr;
			}
		}

		// ***************************
		// ****** SLEEPING STATE ********
		// ***************************

		Sleeping::Sleeping(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
				MotorControlState(pMotorDriver, pLogger)
		{
			m_pMotorDriver->sleepMotor();
			m_stateId = SLEEPING;
			setName("SLEEPING");
			m_pLogger->logString(info, "Motor controller entering SLEEPING state, sleeping motor driver");
		}

		std::unique_ptr<MotorControlState> Sleeping::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case DISABLE_MOTOR:
				case ENABLE_MOTOR:
				case SLEEP:
				case CLEAR_FAULT:
					// do nothing
					return nullptr;

				case MOTOR_COMMAND:
					return nullptr;

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return std::make_unique<Fault>(m_pMotorDriver, m_pLogger);

				case WAKE:
					m_pLogger->logString(info, "Received wake event, waking motor driver");
					return std::make_unique<Disabled>(m_pMotorDriver, m_pLogger);

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller SLEEPING state");
					return nullptr;
			}
		}

		// ***************************
		// ******** RAMP STATE *******
		// ***************************

		Ramp::Ramp(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger, std::unique_ptr<MotorCommand>& pMotorCommand) :
				MotorControlState(pMotorDriver, pLogger),
				m_pMotorCommand{std::move(pMotorCommand)}
		{
			m_stateId = RAMP;
			setName("RAMP");

			if (!m_pMotorCommand.get())
			{
				// no valid command received - exit state right away
				m_stateExitTime = m_stateEntryTime;
				m_pLogger->logString(error, "Entering ramp state, no motor command provided, speed will be unchanged");
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

					m_pLogger->logString(info, logStringStream.str());
				}
			}
		}

		Ramp::~Ramp()
		{

		}

		std::unique_ptr<MotorControlState> Ramp::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case ENABLE_MOTOR:
				case SLEEP:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return nullptr;

				case TICK:
					// check to see if it's time to exit the state
					// note we check if we are at least equal to the state exit time, but not more than 1000 ticks greater; this
					// allows us to tolerate a rollover in the tick timer without having to worry if we miss the exact tick when
					// we should exit
					if (m_pTimebase->getTime_ms() >= m_stateExitTime && m_pTimebase->getTime_ms() < (m_stateExitTime + 1000))
					{
						// ensure we've got the correct final duty cycle
						m_pMotorDriver->setPwmDutyCycle(m_pMotorCommand->pwmDutyCycle);

						// exit to the correct state
						if (m_pMotorCommand->pwmDutyCycle == 0)
							return std::make_unique<Idle>(m_pMotorDriver, m_pLogger);
						else
							return std::make_unique<Running>(m_pMotorDriver, m_pLogger);
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
								// from zero again in the opposite direction
								m_dutyCycleChangeRate *= -1;

								m_pMotorDriver->setPwmDutyCycle(newDutyCycle);
							}
							else
							{
								// this is an unexpected zero crossing, we will just zero the duty cycle and log the incident
								m_pMotorDriver->setPwmDutyCycle(0);
								m_pLogger->logString(warning, "Unexpected zero crossing in motor driver PWM ramp, idling motor");
								return std::make_unique<Idle>(m_pMotorDriver, m_pLogger);
							}
						}
						else
						{
							m_pMotorDriver->setPwmDutyCycle(newDutyCycle);
						}
					}

					return nullptr;

				case MOTOR_COMMAND:
					// We don't process new commands while ramping
					m_pLogger->logString(warning, "Ignoring new motor command received while ramping");

					return nullptr;

				case DISABLE_MOTOR:
					return std::make_unique<Disabled>(m_pMotorDriver, m_pLogger);

				case MOTOR_FAULT:
					return std::make_unique<Fault>(m_pMotorDriver, m_pLogger);

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller RAMP state");
					return nullptr;
			}
		}

		// ***************************
		// ****** RUNNING STATE ******
		// ***************************

		Running::Running(IMotorDriver* pMotorDriver, std::shared_ptr<Logging::ILogger>& pLogger) :
				MotorControlState(pMotorDriver, pLogger)
		{
			std::stringstream logStringStream;
			m_stateId = RUNNING;
			setName("RUNNING");

			logStringStream << "Motor controller entering RUNNING state in " << (m_pMotorDriver->getForwardDirection() ? "forward" : "reverse")
					<< " direction at duty cycle " << m_pMotorDriver->getPwmDutyCycle();
			m_pLogger->logString(info, logStringStream.str());
		}

		std::unique_ptr<MotorControlState> Running::handleEvent(MotorEvent event)
		{
			switch (event.eventType)
			{
				case TICK:
				case ENABLE_MOTOR:
				case WAKE:
				case CLEAR_FAULT:
					// do nothing
					return nullptr;

				case MOTOR_COMMAND:
					if (event.cmd)
						return std::make_unique<Ramp>(m_pMotorDriver, m_pLogger, event.cmd);
					else
					{
						m_pLogger->logString(error, "Motor command event received without valid command object");
						return nullptr;
					}

				case DISABLE_MOTOR:
					// enter disabled state, where motor driver will be disabled
					return std::make_unique<Disabled>(m_pMotorDriver, m_pLogger);

				case MOTOR_FAULT:
					// enter fault state, where motor will be stopped and disabled
					return std::make_unique<Fault>(m_pMotorDriver, m_pLogger);

				case SLEEP:
					// enter sleeping state, where motor driver will be disabled and put to low power sleep state
					return std::make_unique<Sleeping>(m_pMotorDriver, m_pLogger);

				default:
					m_pLogger->logString(error, "Unhandled event type in motor controller IDLE state");
					return nullptr;
			}
		}
	} // namespace MotorControlStates
} // namespace MotorControl
