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

	// update routine to run at regular intervals by parent object
	void PololuG2MotorDriver::update()
	{
		updateEncoder();
		updateMotorDrive();
	}

	void PololuG2MotorDriver::setPwmDutyCycle(int32_t dutyCycle)
	{
		// duty cycle is provided as tenths of a percent of maximum, we convert here to timer ticks
		int32_t newDutyCycle_ticks = dutyCycle*m_maxDutyCycle/1000;
		m_dutyCycle = std::clamp(newDutyCycle_ticks, static_cast<int32_t>(0), m_maxDutyCycle);
	}

	int32_t PololuG2MotorDriver::getPwmDutyCycle() const
	{
		// duty cycle is stored as timer ticks, we convert here to tenths of a percent of maximum drive
		return (m_dutyCycle*1000/m_maxDutyCycle);
	}

	void PololuG2MotorDriver::setMaxDutyCycle(uint16_t maxDutyCycle_pct)
	{
		// max duty cycle is provided in tenths of a percent, we convert here to raw timer counts and update the duty cycle
		// first clamp to the range 0 to 100% (or 0 to 1000 tenths of a percent)
		maxDutyCycle_pct = std::clamp(static_cast<int>(maxDutyCycle_pct), 0, 1000);

		// now convert from tenths of a percent to timer ticks and clamp to the minimum allowed max DC level
		m_maxDutyCycle = std::max(static_cast<int32_t>(maxDutyCycle_pct)*static_cast<int32_t>(DUTY_CYCLE_MAX_LIMIT)/1000,
									DUTY_CYCLE_MAX_MIN_LIMIT);
	}

	uint16_t PololuG2MotorDriver::getMaxDutyCycle() const
	{
		// we store duty cycle in timer ticks, we need to convert here to tenths of a percent of the max duty cycle limit

		return static_cast<uint16_t>(m_maxDutyCycle * 1000 / DUTY_CYCLE_MAX_LIMIT);
	}

	void PololuG2MotorDriver::updateEncoder()
	{
		m_encoderCount = static_cast<int32_t>(TIM1->CNT) - ENCODER_ZERO_COUNT;
	}

	// reset the encoder to the mid-scale zero value
	void PololuG2MotorDriver::zeroEncoder()
	{
		// initialize the encoder counter to this offset value to avoid rolling this over
		TIM1->CNT = static_cast<uint32_t>(ENCODER_ZERO_COUNT);
	}

	// write drive configuratino out to motor driver
	void PololuG2MotorDriver::updateMotorDrive()
	{
		if (m_isSleeping)
		{
			// when sleeping we clear the duty cycle and disable the motor
			m_dutyCycle = 0;
			m_motorEnabled = false;

			// stop the motor
			TIM2->CCR1 = m_dutyCycle;

			// assert the SLP_ pin
			HAL_GPIO_WritePin(SLP__GPIO_Port, SLP__Pin, GPIO_PIN_RESET);
		}
		else
		{
			// deassert SLP_ pin
			HAL_GPIO_WritePin(SLP__GPIO_Port, SLP__Pin, GPIO_PIN_SET);

			if (m_motorEnabled)
			{
				TIM2->CCR1 = m_dutyCycle;
				HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, m_motorDirectionForward ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
			else
			{
				TIM2->CCR1 = 0;
			}
		}
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
