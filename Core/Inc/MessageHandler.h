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
#include "Message.h"

class MessageHandler {
public:
	MessageHandler();
	virtual ~MessageHandler();

	void loadIncomingMessage(Message& msg);
	void processMessages();

private:
	bool parseMessage(const Message& input);
	void buildCommandMap();

	std::queue<Message> m_messageQueue;
	std::unordered_map<uint8_t, std::function<bool(const Message&)>> m_commandMap;
};

#endif /* INC_MESSAGEHANDLER_H_ */
