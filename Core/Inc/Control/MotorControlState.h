/*
 * MotorControlState.h
 *
 *  Created on: Jul 6, 2023
 *      Author: tim
 *
 * Contains implementation for the following motor controller states:
 * - DISABLED:			The motor driver is disabled
 * - SLEEPING:			The motor driver is asleep
 * - FAULT:				The motor driver has reported a fault and is disabled
 * - IDLE: 				The motor driver is enabled but the motor is idle
 * - RUNNING:			The motor is currently running at a constant duty cycle
 * - RAMP:				The motor is ramping between steady state duty cycle settings
 *
 */

#ifndef SRC_CONTROL_MOTORCONTROLSTATE_H_
#define SRC_CONTROL_MOTORCONTROLSTATE_H_

#include <PololuG2MotorDriver.h>
#include "stm32l4xx_hal.h"
#include <string>

namespace MotorControl
{
	enum EventType
	{
		MOTOR_FAULT = 0,
		MOTOR_COMMAND,
		TICK,
		ENABLE_MOTOR,
		DISABLE_MOTOR,
		SLEEP,
		WAKE,
		CLEAR_FAULT
	};

	enum MotorStateIdCode
	{
		UNSET = 0,
		DISABLED,		// Motor driver disabled
		SLEEPING,		// Motor driver sleeping
		FAULT,			// Motor driver has faulted
		IDLE,			// Motor driver is enabled, motor is idle (not spinning)
		RUNNING,		// Motor driver is enabled, motor is running at constant duty cycle
		RAMP			// Motor driver is enabled, motor is ramping to new duty cycle
	};

	typedef struct MotorCommandType
	{
		bool forwardDirection;
		bool directionChange;
		int32_t pwmDutyCycle;
		uint32_t rampTimeTicks;
	} MotorCommand;

	typedef struct MotorEventStruct
	{
		EventType eventType;
		MotorCommand *cmd;
	} MotorEvent;

	class MotorControlState {
	public:
		MotorControlState(PololuG2MotorDriver*);
		virtual ~MotorControlState();

		virtual MotorControlState* handleEvent(MotorEvent) = 0;
		std::string getName() const { return m_stateName; }
		void setName(std::string name) { m_stateName = name; }
		MotorStateIdCode getId() const { return m_stateId; }
		uint32_t timeInState() const;

	private:
		std::string m_stateName{"undefined"};

	protected:
		PololuG2MotorDriver* m_pMotorDriver;
		MotorStateIdCode m_stateId{UNSET};
		uint32_t m_stateEntryTime{0};
	};

	namespace MotorControlStates
	{
		class Disabled : public MotorControlState
		{
		public:
			Disabled(PololuG2MotorDriver*);
			MotorControlState* handleEvent(MotorEvent) override;
		};

		class Idle : public MotorControlState
		{
		public:
			Idle(PololuG2MotorDriver*);
			MotorControlState* handleEvent(MotorEvent) override;
		};

		class Fault : public MotorControlState
		{
		public:
			Fault(PololuG2MotorDriver*);
			MotorControlState* handleEvent(MotorEvent) override;
		};

		class Sleeping : public MotorControlState
		{
		public:
			Sleeping(PololuG2MotorDriver*);
			MotorControlState* handleEvent(MotorEvent) override;
		};

		class Ramp : public MotorControlState
		{
		public:
			Ramp(PololuG2MotorDriver*, MotorCommand*);
			~Ramp();
			MotorControlState* handleEvent(MotorEvent) override;
		private:
			MotorCommand* m_pMotorCommand{nullptr};
			uint32_t m_stateExitTime{0};
			int32_t m_dutyCycleChangeRate{0};
		};

		class Running : public MotorControlState
		{
		public:
			Running(PololuG2MotorDriver*);
			MotorControlState* handleEvent(MotorEvent) override;
		};
	} // namespace MotorControlStates

} // namespace MotorControl
#endif /* SRC_CONTROL_MOTORCONTROLSTATE_H_ */
