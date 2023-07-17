/*
 * Controller.h
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include <MotorControlState.h>
#include <PololuG2MotorDriver.h>

namespace MotorControl
{
	constexpr uint32_t VELOCITY_BUFFER_SIZE = 128;

	class Controller {
	public:
		Controller();

		void tick();
		void updateSpeed();

		void motorFault();

		void enableMotor();
		void disableMotor();
		void sleepDriver();
		void wakeDriver();

		void sendCommand(bool forwardDirection, uint16_t dutyCycle, uint16_t rampTimeMs);
		void idleMotor();

		bool getMotorDirection() const;
		int32_t getMotorPosition() const;
		uint16_t getMotorPwmLevel() const;

		void setMaxDutyCycle(uint16_t maxDutyCycle) { m_motorDriver.setMaxDutyCycle(maxDutyCycle); }
		uint16_t getMaxDutyCycle() const { return m_motorDriver.getMaxDutyCycle(); }

		uint32_t getState() const { return m_state->getId(); }
		bool isDriverSleeping() const { return m_motorDriver.isDriverSleeping(); }
		bool isDriverFaulted() const { return m_motorDriver.isDriverFaulted(); }

		float getMotorSpeed() const { return m_bufferedMotorSpeed; }

	private:
		void updateState(MotorControlState*);

		PololuG2MotorDriver m_motorDriver;
		MotorControlState *m_state{nullptr};

		// motor speed is measured in units of encoder counts per tick
		float m_instantaneousMotorSpeed{0};
		float m_bufferedMotorSpeed{0};
		float m_pMotorSpeedBuffer[VELOCITY_BUFFER_SIZE];
		uint32_t m_speedBufferIdx{0};
		int32_t m_lastEncoderCount{0};
	};

} /* namespace MotorControl */

#endif /* SRC_CONTROLLER_H_ */
