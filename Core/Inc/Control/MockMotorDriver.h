/*
 * MockMotorDriver.h
 *
 *  Created on: Sep 15, 2023
 *      Author: tim
 */

#ifndef INC_CONTROL_MOCKMOTORDRIVER_H_
#define INC_CONTROL_MOCKMOTORDRIVER_H_

#include <IMotorDriver.h>
#include <gmock/gmock.h>

class FakeMotorDriver
{
public:
	// note this ignores the max duty cycle check
	void setPwmDutyCycle(int32_t dutyCycle) { m_pwmDutyCycle = dutyCycle; }
	int32_t getPwmDutyCycle() { return m_pwmDutyCycle; }
	bool getForwardDirection() { return m_bForwardDirection; }
	void setForwardDirection(bool direction) { m_bForwardDirection = direction; }
	uint16_t getMaxDutyCycle() { return 1000; }
	void disableMotor() { m_pwmDutyCycle = 0; }

private:
	int32_t m_pwmDutyCycle{0};
	bool m_bForwardDirection{false};
};

class MockMotorDriver : public IMotorDriver
{
public:
	MOCK_METHOD(void, update, (), (override));
	MOCK_METHOD(void, sleepMotor, (), (override));
	MOCK_METHOD(void, wakeMotor, (), (override));
	MOCK_METHOD(void, enableMotor, (), (override));
	MOCK_METHOD(void, disableMotor, (), (override));
	MOCK_METHOD(bool, isMotorEnabled, (), (const override));
	MOCK_METHOD(bool, isDriverSleeping, (), (const override));
	MOCK_METHOD(bool, isDriverFaulted, (), (const override));
	MOCK_METHOD(bool, getForwardDirection, (), (const override));
	MOCK_METHOD(void, setForwardDirection, (bool), (override));
	MOCK_METHOD(int32_t, getEncoderCounts, (), (const override));
	MOCK_METHOD(float, getShaftRotationCount, (), (const override));
	MOCK_METHOD(int32_t, getPwmDutyCycle, (), (const override));
	MOCK_METHOD(void, setPwmDutyCycle, (int32_t), (override));
	MOCK_METHOD(void, setMaxDutyCycle, (uint16_t), (override));
	MOCK_METHOD(uint16_t, getMaxDutyCycle, (), (const override));

	void DelegateToFake()
	{
		ON_CALL(*this, setPwmDutyCycle).WillByDefault([this](int32_t dutyCycle) {
		  return fake_.setPwmDutyCycle(dutyCycle);
		});
		ON_CALL(*this, getPwmDutyCycle).WillByDefault([this]() {
		  return fake_.getPwmDutyCycle();
		});
		ON_CALL(*this, getForwardDirection).WillByDefault([this]() {
		  return fake_.getForwardDirection();
		});
		ON_CALL(*this, setForwardDirection).WillByDefault([this](bool direction) {
		  return fake_.setForwardDirection(direction);
		});
		ON_CALL(*this, getMaxDutyCycle).WillByDefault([this]() {
		  return fake_.getMaxDutyCycle();
		});
		ON_CALL(*this, disableMotor).WillByDefault([this]() {
		  return fake_.disableMotor();
		});
	}

private:
	FakeMotorDriver fake_;
};


#endif /* INC_CONTROL_MOCKMOTORDRIVER_H_ */
