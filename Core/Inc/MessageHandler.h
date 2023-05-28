/*
 * MessageHandler.h
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#ifndef INC_MESSAGEHANDLER_H_
#define INC_MESSAGEHANDLER_H_

#include <queue>
#include "Message.h"

class MessageHandler {
public:
	MessageHandler();
	virtual ~MessageHandler();

	void loadIncomingMessage(Message& msg);
	void processMessages();

private:
	std::queue<Message> m_messageQueue;
};

#endif /* INC_MESSAGEHANDLER_H_ */
