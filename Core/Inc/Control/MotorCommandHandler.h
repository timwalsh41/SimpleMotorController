/*
 * MotorCommandHandler.h
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_MOTORCOMMANDHANDLER_H_
#define SRC_CONTROL_MOTORCOMMANDHANDLER_H_

#include "Controller.h"
#include "Message.h"
#include <vector>

namespace MotorControl {

class MotorCommandHandler {
public:
	MotorCommandHandler(Controller* controller) : m_pController{controller} {}
	bool enableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool disableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool sleepDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool wakeDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool getMotorPosition(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool getMotorSpeed(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool getMotorDirection(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool rampMotorOn(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool rampMotorOff(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool setMaxDutyCycle(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
	bool injectFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
private:
	Controller* m_pController{nullptr};
};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORCOMMANDHANDLER_H_ */
