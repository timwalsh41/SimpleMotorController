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
#include "BaseLogger.h"
#include "IOutputGateway.h"
#include "IMotorCommandHandler.h"
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

		std::shared_ptr<Logging::BaseLogger> m_pLogger;
		std::shared_ptr<IMotorCommandHandler> m_pMotorCommandHandler;
		std::shared_ptr<IOutputGateway> m_pOutputGateway;
		std::shared_ptr<Sensors::CurrentSensor> m_pCurrentSensor;

		std::queue<Message> m_messageQueue;
		std::unordered_map<uint8_t, std::function<bool(const Message&, std::vector<uint8_t>&)>> m_commandMap;
	};

} /* namespace MessageHandling */
#endif /* INC_MESSAGEHANDLER_H_ */
