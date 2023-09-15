/*
 * MotorControlConstants.h
 *
 *  Created on: Sep 14, 2023
 *      Author: tim
 */

#ifndef INC_CONTROL_MOTORCONTROLCONSTANTS_H_
#define INC_CONTROL_MOTORCONTROLCONSTANTS_H_

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
} // namespace MotorControl

#endif /* INC_CONTROL_MOTORCONTROLCONSTANTS_H_ */
