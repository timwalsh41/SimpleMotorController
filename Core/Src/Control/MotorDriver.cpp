/*
 * MotorDriver.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#include "main.h"
#include "MotorDriver.h"
#include <algorithm>

// TODO: Handle FLT signal

namespace MotorControl
{

	MotorDriver::MotorDriver() {
		// TODO Auto-generated constructor stub
		zeroEncoder();
	}

	MotorDriver::~MotorDriver() {
		// TODO Auto-generated destructor stub
	}

	// update routine to run at regular intervals by parent object
	void MotorDriver::update()
	{
		updateEncoder();
		updateMotorDrive();
	}

	void MotorDriver::setPwmDutyCycle(int32_t dutyCycle)
	{
		m_dutyCycle = std::clamp(dutyCycle, static_cast<int32_t>(0), m_maxDutyCycle);
	}

	void MotorDriver::updateEncoder()
	{
		m_encoderCount = static_cast<int32_t>(TIM1->CNT) - ENCODER_ZERO_COUNT;
	}

	// reset the encoder to the mid-scale zero value
	void MotorDriver::zeroEncoder()
	{
		// initialize the encoder counter to this offset value to avoid rolling this over
		TIM1->CNT = static_cast<uint32_t>(ENCODER_ZERO_COUNT);
	}

	// write drive configuratino out to motor driver
	void MotorDriver::updateMotorDrive()
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

	bool MotorDriver::isDriverFaulted() const
	{
		return HAL_GPIO_ReadPin(FLT__GPIO_Port, FLT__Pin) == GPIO_PIN_RESET ? true : false;
	}

	// return the encoder position in rotations of the output shaft from the zero position
	float MotorDriver::getShaftRotationCount() const
	{
		return static_cast<float>(m_encoderCount)/ENCODER_PULSES_PER_SHAFT_REVOLUTION;
	}

} /* namespace MotorControl */
