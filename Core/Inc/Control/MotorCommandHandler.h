/*
 * MotorCommandHandler.h
 *
 *  Created on: Jul 17, 2023
 *      Author: tim
 */

#ifndef SRC_CONTROL_MOTORCOMMANDHANDLER_H_
#define SRC_CONTROL_MOTORCOMMANDHANDLER_H_

#include "IMotorCommandHandler.h"
#include "Controller.h"
#include "Message.h"
#include "BaseLogger.h"
#include <vector>
#include <memory>

namespace MotorControl {

class MotorCommandHandler : public IMotorCommandHandler {
public:
	MotorCommandHandler(std::shared_ptr<Controller>& pController);
	bool enableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool disableMotor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool sleepDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool wakeDriver(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool getMotorPosition(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool getMotorSpeed(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool getMotorDirection(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool rampMotorOn(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool rampMotorOff(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool setMaxDutyCycle(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool injectFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool clearFault(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
	bool getStatus(const MessageHandling::Message& msg, std::vector<uint8_t>& payload) override;
private:
	std::shared_ptr<Controller> m_pController;
	std::shared_ptr<Logging::BaseLogger> m_pLogger;
};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORCOMMANDHANDLER_H_ */
