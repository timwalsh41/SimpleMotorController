/*
 * MessageHandler.h
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#ifndef INC_MESSAGEHANDLER_H_
#define INC_MESSAGEHANDLER_H_

#include <queue>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include "Message.h"
#include "IMessageHandler.h"
#include "ILogger.h"
#include "OutputGateway.h"
#include "Controller.h"
#include "MotorCommandHandler.h"
#include "CurrentSensor.h"

namespace MessageHandling
{

	class MessageHandler : public IMessageHandler
	{
	public:
		MessageHandler();
		virtual ~MessageHandler();

		void loadIncomingMessage(Message& msg) override;
		void processMessages() override;

	private:
		void buildCommandMap();

		std::shared_ptr<Logging::ILogger> m_pLogger;
		std::shared_ptr<MotorControl::Controller> m_pController;
		std::shared_ptr<MotorControl::MotorCommandHandler> m_pMotorCommandHandler;
		std::shared_ptr<MessageHandling::OutputGateway> m_pOutputGateway;
		std::shared_ptr<Sensors::CurrentSensor> m_pCurrentSensor;


		std::queue<Message> m_messageQueue;
		std::unordered_map<uint8_t, std::function<bool(const Message&, std::vector<uint8_t>&)>> m_commandMap;
	};

} /* namespace MessageHandling */
#endif /* INC_MESSAGEHANDLER_H_ */
