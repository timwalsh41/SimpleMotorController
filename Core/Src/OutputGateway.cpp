/*
 * OutputGateway.cpp
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#include "OutputGateway.h"
#include "Utility.h"
#include "SingletonFactory.h"
#include "CommandInterface.h"
#include <cstring>

namespace MessageHandling
{

	OutputGateway::OutputGateway(IUartWrapper* pUartWrapper) :
			m_pUartWrapper{pUartWrapper}
	{
		SingletonFactory factory;
		m_pLogger = factory.retrieve<Logging::BaseLogger>();
	}

	OutputGateway::~OutputGateway()
	{

	}

	void OutputGateway::sendMessage(Message& msg)
	{
		m_messageQueue.push(std::move(msg));
	}

	void OutputGateway::transmitString(std::string& outString)
	{
		setTransmitting();
		m_pUartWrapper->transmitString(outString);

		// adding a bit of delay here seems to keep the transmitted array from being corrupted
		// Utility::delayCycles(1000);
	}

	void OutputGateway::transmitMessage(const Message& msg)
	{
		std::string fullMessage{};

		if (messageToStringWithStartStop(fullMessage, msg))
		{
			transmitString(fullMessage);
		}
		else
			m_pLogger->logString(error, "Unable to serialize and transmit output message");
	}

	void OutputGateway::processMessages()
	{
		if (!m_receiving)
			// nothing to do while we're in the middle of a transmission
			return;

		// send one message or log entry, if any exist, we only send one per call
		// to allow time for transmission
		if (!m_messageQueue.empty())
		{
			transmitMessage(m_messageQueue.front());
			m_messageQueue.pop();
		}
	}

	bool OutputGateway::messageToStringWithStartStop(std::string& outputString, const Message& inputMessage) const
	{
		std::vector<uint8_t> msgBytes;
		outputString.clear();

		// the output string size is the message payload size, plus the minimum message length (command code, CRC, etc.)
		// plus the start sequence and stop sequence lengths
		outputString.reserve(MessageHandling::MINIMUM_MESSAGE_LENGTH +
				inputMessage.getPayload().size() +
				START_SEQUENCE_LENGTH +
				STOP_SEQUENCE_LENGTH);

		if (!inputMessage.toByteArray(msgBytes))
		{
			m_pLogger->logString(error, "Unable to convert message to byte array for transmission");
			return false;
		}

		outputString.insert(outputString.end(), &START_SEQUENCE[0], &START_SEQUENCE[START_SEQUENCE_LENGTH]);
		outputString.insert(outputString.end(), msgBytes.begin(), msgBytes.end());
		outputString.insert(outputString.end(), &STOP_SEQUENCE[0], &STOP_SEQUENCE[STOP_SEQUENCE_LENGTH]);

		return true;
	}

} /* namespace MessageHandling */
