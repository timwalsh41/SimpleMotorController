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
#include "ILogger.h"
#include <vector>
#include <memory>

namespace MotorControl {

class MotorCommandHandler {
public:
	MotorCommandHandler(std::shared_ptr<Controller>& pController);
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
	bool clearFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);
private:
	std::shared_ptr<Controller> m_pController;
	std::shared_ptr<Logging::ILogger> m_pLogger;
};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORCOMMANDHANDLER_H_ */
