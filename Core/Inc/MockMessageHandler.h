/*
 * MockMessageHandler.h
 *
 *  Created on: Aug 25, 2023
 *      Author: tim
 */

#ifndef INC_MOCKMESSAGEHANDLER_H_
#define INC_MOCKMESSAGEHANDLER_H_

#include <IMessageHandler.h>
#include <Message.h>
#include <gmock/gmock.h>

class MockMessageHandler: public MessageHandling::IMessageHandler {
public:
	MOCK_METHOD(void, loadIncomingMessage, (MessageHandling::Message& msg), (override));
	MOCK_METHOD(void, processMessages, (), (override));
};

#endif /* INC_MOCKMESSAGEHANDLER_H_ */
