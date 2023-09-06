/*
 * PololuG2MotorDriver.h
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

	// This value must match the setting in CubeMX for TIM2 counter period
	constexpr int32_t DUTY_CYCLE_MAX_LIMIT = 1600;

	// the minimum value we'll allow for the max duty cycle setting
	constexpr int32_t DUTY_CYCLE_MAX_MIN_LIMIT = 400;

	// we will zero the encoder counter at this value, and then subtract off this
	// offset to report a value centered at zero
	constexpr int32_t ENCODER_ZERO_COUNT = 1073741824;

	constexpr uint32_t MOTOR_GEAR_RATIO = 50;
	constexpr uint32_t ENCODER_PULSES_PER_MOTOR_REVOLUTION = 64;
	constexpr uint32_t ENCODER_PULSES_PER_SHAFT_REVOLUTION = MOTOR_GEAR_RATIO *
															 ENCODER_PULSES_PER_MOTOR_REVOLUTION;

	constexpr uint16_t DEFAULT_RAMP_TIME_MS = 250;

	class PololuG2MotorDriver {
	public:
		PololuG2MotorDriver();

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
		int32_t getPwmDutyCycle() const;

		// set maximum duty cycle in percent of the limit; we actually pass the value
		// in tenths of a percent
		void setMaxDutyCycle(uint16_t maxDutyCycle_pct);
		uint16_t getMaxDutyCycle() const;

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
