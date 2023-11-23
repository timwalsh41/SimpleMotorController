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

namespace MessageHandling
{
class FakeOutputGateway
{
public:
	void sendMessage(MessageHandling::Message& msg)
	{
		std::cout << "Consuming message\n";
		m_msg = std::move(msg);
	}

	uint8_t processMessage()
	{
		if (m_msg.getPayload().size() == 1)
			return m_msg.getPayload()[0];

		return 0;
	}

private:
	MessageHandling::Message m_msg;
};

class MockOutputGateway : public IOutputGateway
{
public:
	MOCK_METHOD(void, sendMessage, (MessageHandling::Message&), (override));
	MOCK_METHOD(void, processMessages, (), (override));
	MOCK_METHOD(uint8_t, processMessage, (), ());

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

} /* namespace MessageHandling */

#endif /* INC_MOCKOUTPUTGATEWAY_H_ */
