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
#include <memory>
#include "Message.h"
#include "CommandInterface.h"
#include "ILogger.h"
#include "IUartWrapper.h"

namespace MessageHandling
{

	class OutputGateway
	{
	public:
		OutputGateway(IUartWrapper*);
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
		bool messageToStringWithStartStop(std::string&, const Message&) const;

		bool m_receiving = true;

		std::queue<std::string> m_logStringQueue{};
		std::queue<Message> m_messageQueue{};
		std::shared_ptr<Logging::ILogger> m_pLogger;
		IUartWrapper* m_pUartWrapper;
	};

} /* namespace MessageHandling */

#endif /* INC_OUTPUTGATEWAY_H_ */
