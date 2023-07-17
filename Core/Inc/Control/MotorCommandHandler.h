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

namespace MotorControl {

class MotorCommandHandler {
public:
	MotorCommandHandler(Controller* controller) : m_pController{controller} {}
	bool enableMotor(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
	bool disableMotor(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
	bool sleepDriver(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
	bool wakeDriver(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
	bool getMotorPosition(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
	bool getMotorSpeed(const MessageHandling::Message& msg, uint8_t* payload, uint8_t& payloadLength);
private:
	Controller* m_pController{nullptr};
};

} /* namespace MotorControl */

#endif /* SRC_CONTROL_MOTORCOMMANDHANDLER_H_ */
