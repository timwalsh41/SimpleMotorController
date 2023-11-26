/*
 * PololuG2MotorDriver.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#include <PololuG2MotorDriver.h>
#include "main.h"
#include <algorithm>

namespace MotorControl
{

	PololuG2MotorDriver::PololuG2MotorDriver() {
		zeroEncoder();
	}

	void PololuG2MotorDriver::disableMotor()
	{
		m_dutyCycle = 0;
		m_motorEnabled = false;

		// set the HW duty cycle immediately, don't wait for next update
		setHwDutyCycle(0);
	}

	// update routine to run at regular intervals by parent object
	void PololuG2MotorDriver::update()
	{
		updateEncoder();
		updateMotorDrive();
	}

	void PololuG2MotorDriver::setPwmDutyCycle(int32_t dutyCycle)
	{
		// duty cycle is provided/stored as tenths of a percent of the limit, capped to max percent of limit
		m_dutyCycle = std::clamp(dutyCycle, static_cast<int32_t>(0), m_maxDutyCycle);
	}

	int32_t PololuG2MotorDriver::getPwmDutyCycle() const
	{
		// duty cycle is provided/stored as tenths of a percent of maximum
		return m_dutyCycle;
	}

	void PololuG2MotorDriver::setMaxDutyCycle(uint16_t maxDutyCycle_pct)
	{
		// max duty cycle is the maximum percent of the limit
		m_maxDutyCycle = std::clamp(static_cast<int>(maxDutyCycle_pct), static_cast<int>(DUTY_CYCLE_MAX_MIN_LIMIT), 1000);
	}

	uint16_t PololuG2MotorDriver::getMaxDutyCycle() const
	{
		return static_cast<uint16_t>(m_maxDutyCycle);
	}

	void PololuG2MotorDriver::updateEncoder()
	{
		m_encoderCount = static_cast<int32_t>(TIM1->CNT) - static_cast<int32_t>(ENCODER_ZERO_COUNT);
	}

	// reset the encoder to the mid-scale zero value
	void PololuG2MotorDriver::zeroEncoder()
	{
		// initialize the encoder counter to this offset value to avoid rolling this over
		TIM1->CNT = ENCODER_ZERO_COUNT;
	}

	// write drive configuration out to motor driver
	void PololuG2MotorDriver::updateMotorDrive()
	{
		if (m_isSleeping)
		{
			// when sleeping we clear the duty cycle and disable the motor
			m_dutyCycle = 0;
			m_motorEnabled = false;

			// stop the motor
			setHwDutyCycle(0);

			// assert the SLP_ pin
			HAL_GPIO_WritePin(SLP__GPIO_Port, SLP__Pin, GPIO_PIN_RESET);
		}
		else
		{
			// deassert SLP_ pin
			HAL_GPIO_WritePin(SLP__GPIO_Port, SLP__Pin, GPIO_PIN_SET);

			if (m_motorEnabled)
			{
				// convert duty cycle from tenths of a percent of the limit to timer ticks
				setHwDutyCycle(static_cast<int32_t>(m_dutyCycle*DUTY_CYCLE_LIMIT/1000));
				HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, m_motorDirectionForward ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
			else
			{
				setHwDutyCycle(0);
			}
		}
	}

	// write the duty cycle setting to the timer 2 control register
	// no input checking here on this private function
	void PololuG2MotorDriver::setHwDutyCycle(int32_t dutyCycle)
	{
		// write the duty cycle directly to the channel 1 CCR register
		TIM2->CCR1 = dutyCycle;
	}

	bool PololuG2MotorDriver::isDriverFaulted() const
	{
		return HAL_GPIO_ReadPin(FLT__GPIO_Port, FLT__Pin) == GPIO_PIN_RESET ? true : false;
	}

	// return the encoder position in rotations of the output shaft from the zero position
	float PololuG2MotorDriver::getShaftRotationCount() const
	{
		return static_cast<float>(m_encoderCount)/ENCODER_PULSES_PER_SHAFT_REVOLUTION;
	}

} /* namespace MotorControl */
