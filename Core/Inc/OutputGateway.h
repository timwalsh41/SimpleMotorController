/*
 * OutputGateway.h
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#ifndef INC_OUTPUTGATEWAY_H_
#define INC_OUTPUTGATEWAY_H_

#include <queue>
#include <string>
#include <string_view>
#include <memory>
#include "Message.h"
#include "CommandInterface.h"
#include "ILogger.h"

namespace MessageHandling
{

	class OutputGateway
	{
	public:
		OutputGateway();
		~OutputGateway();
		OutputGateway(const OutputGateway&) = delete;
		OutputGateway& operator=(const OutputGateway&) = delete;

		void addLogString(std::string& msg);
		void sendMessage(const Message& msg);
		void processOutgoingMessages();
		void setReceiving() { m_receiving = true; }
		void setTransmitting() { m_receiving = false; }

	private:
		void transmitString(const std::string& outString);
		void transmitMessage(const Message& outMessage);
		void addStartStopSequence(const uint8_t* inputArray, uint32_t inputLength, uint8_t** outputArray, uint32_t& outputLength) const;

		bool m_receiving = true;

		std::queue<std::string> m_logStringQueue{};
		std::queue<Message> m_messageQueue{};
		std::shared_ptr<Logging::ILogger> m_pLogger;
	};

} /* namespace MessageHandling */

#endif /* INC_OUTPUTGATEWAY_H_ */
