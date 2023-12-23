/*
 * IMotorDriver.h
 *
 *  Created on: Sep 13, 2023
 *      Author: tim
 */

#ifndef INC_CONTROL_IMOTORDRIVER_H_
#define INC_CONTROL_IMOTORDRIVER_H_

// TO DO:
// - Update Pololu G2 motor driver to inherit from IMotorDriver
// - Update Controller to take an IMotorDriver* as an injected dependency
// - Update Controller & MotorControlState to refer to IMotorDriver object/header; remove all references to Pololu G2 motor driver

#include "stdint.h"

namespace MotorControl
{

	class IMotorDriver
	{
	public:
		virtual ~IMotorDriver() {}

		virtual void update() = 0;

		virtual void sleepMotor() = 0;
		virtual void wakeMotor() = 0;
		virtual void enableMotor() = 0;
		virtual void disableMotor() = 0;
		virtual bool isMotorEnabled() const = 0;
		virtual bool isDriverSleeping() const = 0;
		virtual bool isDriverFaulted() const = 0;
		virtual bool getForwardDirection() const = 0;
		virtual void setForwardDirection(bool) = 0;

		virtual int32_t getEncoderCounts() const = 0;
		virtual float getShaftRotationCount() const = 0;
		virtual void zeroEncoder() = 0;
		virtual void zeroRotationCount() = 0;

		virtual int32_t getPwmDutyCycle() const = 0;
		virtual void setPwmDutyCycle(int32_t) = 0;
		virtual void setMaxDutyCycle(uint16_t) = 0;
		virtual uint16_t getMaxDutyCycle() const = 0;
	};

} /* namespace MotorControl*/

#endif /* INC_CONTROL_IMOTORDRIVER_H_ */
