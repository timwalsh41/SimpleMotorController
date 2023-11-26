/*
 * PololuG2MotorDriver.h
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_MOTORDRIVER_H_
#define SRC_CONTROL_MOTORDRIVER_H_

#include "stdint.h"
#include "IMotorDriver.h"
#include "MotorControlConstants.h"

namespace MotorControl
{

	class PololuG2MotorDriver : public IMotorDriver
	{
	public:
		PololuG2MotorDriver();

		void update() override;

		void enableMotor() override { m_dutyCycle = 0; m_motorEnabled = true; }
		void disableMotor() override;
		bool isMotorEnabled() const override { return m_motorEnabled; }

		void setForwardDirection(bool isForward) override { m_motorDirectionForward = isForward; }
		bool getForwardDirection() const override { return m_motorDirectionForward; }

		void sleepMotor() override { m_isSleeping = true; }
		void wakeMotor() override { m_isSleeping = false; }
		bool isDriverSleeping() const override { return m_isSleeping; }
		bool isDriverFaulted() const override;

		void setPwmDutyCycle(int32_t dutyCycle) override;
		int32_t getPwmDutyCycle() const override;

		// set maximum duty cycle in percent of the limit; we actually pass the value
		// in tenths of a percent
		void setMaxDutyCycle(uint16_t maxDutyCycle_pct) override;
		uint16_t getMaxDutyCycle() const override;

		int32_t getEncoderCounts() const override { return m_encoderCount; }
		float getShaftRotationCount() const override;

		void zeroEncoder();

	private:
		bool m_motorEnabled{false};
		bool m_motorDirectionForward{true};
		bool m_isSleeping{false};

		int32_t m_dutyCycle{0};			// stored as tenths of a percent of the limit
		int32_t m_encoderCount{0};
		int32_t m_maxDutyCycle{750};	// stored as tenths of a percent of the limit

		void setHwDutyCycle(int32_t);

		void updateEncoder();
		void updateMotorDrive();
	};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORDRIVER_H_ */
