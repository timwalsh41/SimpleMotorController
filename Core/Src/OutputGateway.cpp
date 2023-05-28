/*
 * OutputGateway.cpp
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#include "stm32l4xx_hal.h"
#include "OutputGateway.h"

extern UART_HandleTypeDef huart2;

OutputGateway::OutputGateway()
{

}

OutputGateway::~OutputGateway()
{

}

void OutputGateway::logString(LogLevel level, const std::string& msg)
{
	std::string outputString = "[" + std::to_string(HAL_GetTick()) + "]" + \
							std::string(logLevelString[level]) + ": " + msg + '\n';
	m_logStringQueue.push(outputString);

	/*
	if (m_receiving)
	{
		sendStringOutput(outputString);
	}
	else
		m_logStringQueue.push(outputString);
	*/

}

void OutputGateway::sendMessage(const Message& msg)
{
	if (m_receiving)
		sendMessageOutput(msg);
	else
		m_messageQueue.push(msg);
}

void OutputGateway::sendStringOutput(const std::string& outString)
{
	setTransmitting();
	HAL_UART_Transmit_IT(&huart2, (const uint8_t*) outString.c_str(), outString.length());
}

void OutputGateway::sendMessageOutput(const Message& outMessage)
{
	setTransmitting();

	// TODO: finish this function
}

void OutputGateway::processOutgoingMessages()
{
	if (!m_receiving)
		// nothing to do while we're in the middle of a transmission
		return;

	// send one message or log entry, if any exist, we only send one per call
	// to allow for transmission
	if (!m_messageQueue.empty())
	{
		sendMessageOutput(m_messageQueue.front());
		// TODO: make sure lifetime of the message is OK here, once we pop
		// it will get destroyed
		m_messageQueue.pop();
	}
	else if (!m_logStringQueue.empty())
	{
		sendStringOutput(m_logStringQueue.front());
		// TODO: make sure lifetime of the string is OK here, once we
		// pop it will get destroyed
		m_logStringQueue.pop();
	}
}
