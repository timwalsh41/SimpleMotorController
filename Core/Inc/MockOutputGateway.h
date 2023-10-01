/*
 * MockOutputGateway.h
 *
 *  Created on: Sep 26, 2023
 *      Author: tim
 */

#ifndef INC_MOCKOUTPUTGATEWAY_H_
#define INC_MOCKOUTPUTGATEWAY_H_


#include <IOutputGateway.h>
#include <gmock/gmock.h>
#include <iostream>

enum MessageProcessingErrorCode
{
	NO_ERROR = 0,
	BAD_CRC
};

class FakeOutputGateway
{
public:
	void sendMessage(MessageHandling::Message& msg)
	{
		std::cout << "Consuming message\n";
		m_msg = std::move(msg);
	}

	MessageProcessingErrorCode processMessage()
	{
		if (!m_msg.checkCRC())
			return BAD_CRC;

		return NO_ERROR;
	}

private:
	MessageHandling::Message m_msg;
};



class MockOutputGateway : public IOutputGateway
{
public:
	MOCK_METHOD(void, addLogString, (std::string&), (override));
	MOCK_METHOD(void, sendMessage, (MessageHandling::Message&), (override));
	MOCK_METHOD(void, processOutgoingMessages, (), (override));
	MOCK_METHOD(MessageProcessingErrorCode, processMessage, (), ());

	void DelegateToFake()
	{
		ON_CALL(*this, sendMessage).WillByDefault([this](MessageHandling::Message& msg) {
			return fake_.sendMessage(msg);
		});
		ON_CALL(*this, processMessage).WillByDefault([this]() {
			return fake_.processMessage();
		});
	}
private:
	FakeOutputGateway fake_;
};


#endif /* INC_MOCKOUTPUTGATEWAY_H_ */
