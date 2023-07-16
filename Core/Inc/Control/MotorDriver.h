/*
 * MotorDriver.h
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_MOTORDRIVER_H_
#define SRC_CONTROL_MOTORDRIVER_H_

#include "stdint.h"

namespace MotorControl
{

	constexpr uint16_t CURRENT_RAW_TO_MILLIAMP = 1;
	constexpr int32_t DEFAULT_MAX_DUTY_CYCLE = 1200;

	// we will zero the encoder counter at this value, and then subtract off this
	// offset to report a value centered at zero
	constexpr int32_t ENCODER_ZERO_COUNT = 1073741824;

	constexpr uint32_t MOTOR_GEAR_RATIO = 50;
	constexpr uint32_t ENCODER_PULSES_PER_MOTOR_REVOLUTION = 64;
	constexpr uint32_t ENCODER_PULSES_PER_SHAFT_REVOLUTION = MOTOR_GEAR_RATIO *
															 ENCODER_PULSES_PER_MOTOR_REVOLUTION;

	constexpr uint16_t DEFAULT_RAMP_TIME_MS = 250;

	class MotorDriver {
	public:
		MotorDriver();
		~MotorDriver();

		void update();

		void enableMotor() { m_dutyCycle = 0; m_motorEnabled = true; }
		void disableMotor() { m_dutyCycle = 0; m_motorEnabled = false; }
		bool isMotorEnabled() const { return m_motorEnabled; }

		void setForwardDirection(bool isForward) { m_motorDirectionForward = isForward; }
		bool getForwardDirection() const { return m_motorDirectionForward; }

		void sleepMotor() { m_isSleeping = true; }
		void wakeMotor() { m_isSleeping = false; }
		bool isDriverSleeping() const { return m_isSleeping; }
		bool isDriverFaulted() const;

		void setPwmDutyCycle(int32_t dutyCycle);
		int32_t getPwmDutyCycle() const { return m_dutyCycle; }

		void setMaxDutyCycle(uint16_t maxDutyCycle) { m_maxDutyCycle = static_cast<int32_t>(maxDutyCycle); }
		uint16_t getMaxDutyCycle() const { return static_cast<uint16_t>(m_maxDutyCycle); }

		int32_t getEncoderCounts() const { return m_encoderCount; }
		float getShaftRotationCount() const;

	private:
		bool m_motorEnabled{false};
		bool m_motorDirectionForward{true};
		bool m_isSleeping{false};

		int32_t m_dutyCycle{0};
		int32_t m_encoderCount{0};
		int32_t m_maxDutyCycle{DEFAULT_MAX_DUTY_CYCLE};

		void updateEncoder();
		void zeroEncoder();
		void updateMotorDrive();

	};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORDRIVER_H_ */
