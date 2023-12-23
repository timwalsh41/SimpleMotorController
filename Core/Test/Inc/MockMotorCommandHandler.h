/*
 * MockMotorCommandHandler.h
 *
 *  Created on: Sep 25, 2023
 *      Author: tim
 */

#ifndef INC_CONTROL_MOCKMOTORCOMMANDHANDLER_H_
#define INC_CONTROL_MOCKMOTORCOMMANDHANDLER_H_

#include <IMotorCommandHandler.h>
#include <gmock/gmock.h>

namespace MotorControl
{

	class MockMotorCommandHandler : public IMotorCommandHandler
	{
	public:
		MOCK_METHOD(bool, enableMotor, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, disableMotor, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, sleepDriver, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, wakeDriver, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getMotorPosition, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getMotorRotation, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getMotorPwm, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getMotorSpeed, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getMotorDirection, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, zeroMotorPosition, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, zeroMotorRotation, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, rampMotorOn, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, rampMotorOff, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, setMaxDutyCycle, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, injectFault, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, clearFault, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
		MOCK_METHOD(bool, getStatus, (const MessageHandling::Message&, std::vector<uint8_t>&), (override));
	};

} /* namespace MotorControl */

#endif /* INC_CONTROL_MOCKMOTORCOMMANDHANDLER_H_ */
