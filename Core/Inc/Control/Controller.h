/*
 * Controller.h
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROLLER_H_
#define SRC_CONTROLLER_H_

#include <memory>
#include "MotorControlState.h"
#include "IMotorDriver.h"
#include "BaseLogger.h"

namespace MotorControl
{
	constexpr uint32_t VELOCITY_BUFFER_SIZE = 128;

	class Controller {
	public:
		Controller(IMotorDriver*);

		void tick();
		void updateSpeed();

		void injectFault();
		void clearFault();

		void enableMotor();
		void disableMotor();
		void sleepDriver();
		void wakeDriver();

		void sendCommand(bool forwardDirection, uint16_t dutyCycle, uint16_t rampTimeMs);
		void idleMotor();

		bool getMotorDirection() const { return m_pMotorDriver->getForwardDirection(); }
		int32_t getMotorPosition() const { return m_pMotorDriver->getEncoderCounts(); }
		float getMotorRotation() const { return m_pMotorDriver->getShaftRotationCount(); }
		uint16_t getMotorPwmLevel() const { return static_cast<uint16_t>(m_pMotorDriver->getPwmDutyCycle()); }

		void zeroMotorEncoder() const { m_pMotorDriver->zeroEncoder(); }
		void zeroMotorRotation() const { m_pMotorDriver->zeroRotationCount(); }

		void setMaxDutyCycle(uint16_t maxDutyCycle) { m_pMotorDriver->setMaxDutyCycle(maxDutyCycle); }
		uint16_t getMaxDutyCycle() const { return m_pMotorDriver->getMaxDutyCycle(); }
		uint16_t getDutyCycle() const { return m_pMotorDriver->getPwmDutyCycle(); }

		uint32_t getState() const { return m_state->getId(); }
		uint32_t getTimeInState() const { return m_state->timeInState(); }
		bool isDriverSleeping() const { return m_pMotorDriver->isDriverSleeping(); }
		bool isDriverFaulted() const { return m_pMotorDriver->isDriverFaulted(); }

		float getMotorSpeed() const { return m_bufferedMotorSpeed; }

	private:
		void updateState(std::unique_ptr<MotorControlState>&);

		IMotorDriver* m_pMotorDriver;
		std::unique_ptr<MotorControlState> m_state{nullptr};

		// motor speed is measured in units of encoder counts per tick
		float m_instantaneousMotorSpeed{0};
		float m_bufferedMotorSpeed{0};
		float m_pMotorSpeedBuffer[VELOCITY_BUFFER_SIZE];
		uint32_t m_speedBufferIdx{0};
		int32_t m_lastEncoderCount{0};

		std::shared_ptr<Logging::BaseLogger> m_pLogger;
	};

} /* namespace MotorControl */

#endif /* SRC_CONTROLLER_H_ */
