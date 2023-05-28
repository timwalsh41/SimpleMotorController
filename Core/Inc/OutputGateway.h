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
#include "Message.h"

enum LogLevel {
	debug = 0,
	info,
	warning,
	error,
	fatal
};

class OutputGateway
{
public:
	OutputGateway();
	~OutputGateway();
	OutputGateway(const OutputGateway&) = delete;

	void logString(LogLevel level, const std::string& msg);
	void sendMessage(const Message& msg);
	void processOutgoingMessages();
	void setReceiving() { m_receiving = true; }
	void setTransmitting() { m_receiving = false; }

private:
	void sendStringOutput(const std::string& outString);
	void sendMessageOutput(const Message& outMessage);
	bool m_receiving = true;
	static constexpr std::string_view logLevelString[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
	std::queue<std::string> m_logStringQueue;
	std::queue<Message> m_messageQueue;
};



#endif /* INC_OUTPUTGATEWAY_H_ */
