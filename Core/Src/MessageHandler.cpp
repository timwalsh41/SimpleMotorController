/*
 * MessageHandler.cpp
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#include <MessageHandler.h>

MessageHandler::MessageHandler() {
	// TODO Auto-generated constructor stub

}

MessageHandler::~MessageHandler() {
	// TODO Auto-generated destructor stub
}

void MessageHandler::loadIncomingMessage(Message& msg)
{
	m_messageQueue.push(std::move(msg));
}

void MessageHandler::processMessages()
{

}
