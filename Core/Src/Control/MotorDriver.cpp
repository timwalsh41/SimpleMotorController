/*
 * MotorDriver.cpp
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#include "main.h"
#include "MotorDriver.h"

MotorDriver::MotorDriver() {
	// TODO Auto-generated constructor stub

}

MotorDriver::~MotorDriver() {
	// TODO Auto-generated destructor stub
}

void MotorDriver::initialize()
{
	// see MotorControl.c initMotorControl()
}

uint16_t MotorDriver::getCurrentmA()
{
	return getCurrentRaw() * CURRENT_RAW_TO_MILLIAMP;
}

uint16_t MotorDriver::getCurrentRaw()
{
	return 0;
}

void MotorDriver::setPwmDutyCycle(uint16_t dutyCycle)
{

}

void MotorDriver::setDirection(bool isForward)
{
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, isForward ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int32_t MotorDriver::readEncoder()
{
	return 0;
}
