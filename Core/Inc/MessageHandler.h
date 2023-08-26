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
#include "Message.h"
#include "IMessageHandler.h"

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

		std::queue<Message> m_messageQueue;
		std::unordered_map<uint8_t, std::function<bool(const Message&, std::vector<uint8_t>&)>> m_commandMap;
	};

} /* namespace MessageHandling */
#endif /* INC_MESSAGEHANDLER_H_ */
