/*
 * MotorDriver.h
 *
 *  Created on: Jun 2, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_MOTORDRIVER_H_
#define SRC_CONTROL_MOTORDRIVER_H_

#include "stdint.h"

constexpr uint16_t CURRENT_RAW_TO_MILLIAMP = 1;

class MotorDriver {
public:
	MotorDriver();
	~MotorDriver();

	void initialize();

	void enableMotor() { m_motorEnabled = true; }
	void disableMotor() { m_motorEnabled = false; }

	void setDirection(bool isForward);
	void setPwmDutyCycle(uint16_t dutyCycle);

	int32_t readEncoder();
	uint16_t getCurrentmA();

private:
	bool m_motorEnabled{false};

	uint16_t getCurrentRaw();
};

#endif /* SRC_CONTROL_MOTORDRIVER_H_ */
