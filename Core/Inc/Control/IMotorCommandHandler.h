/*
 * MotorCommandHandler.h
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_IMOTORCOMMANDHANDLER_H_
#define SRC_CONTROL_IMOTORCOMMANDHANDLER_H_

#include "Message.h"
#include <vector>
#include <memory>

namespace MotorControl
{
	class IMotorCommandHandler {
	public:
		~IMotorCommandHandler() {}
		virtual bool enableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool disableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool sleepDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool wakeDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool getMotorPosition(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool getMotorPwm(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool getMotorSpeed(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool getMotorDirection(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool rampMotorOn(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool rampMotorOff(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool setMaxDutyCycle(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool injectFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool clearFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
		virtual bool getStatus(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) = 0;
	};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORCOMMANDHANDLER_H_ */
