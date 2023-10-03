/*
 * SerialParser.cpp
 *
 *  Created on: May 24, 2023
 *      Author: tim
 */

#include "SerialParser.h"
#include "Message.h"
#include "MessageHandler.h"
#include "SingletonFactory.h"
#include <cstring>

namespace MessageHandling
{
	template <typename T>
	bool SerialParser::sequenceMatch(const char* refSequence, T& compSequence, size_t refSequenceLength)
	{
		if (refSequenceLength != compSequence.size())
		{
			m_pLogger->logString(warning, "sequenceMatch: size mismatch between reference sequence and candidate sequence");
			return false;
		}

		size_t i = 0;
		for (uint8_t c : compSequence)
		{
			if (refSequence[i++] != c)
				return false;
		}

		return true;
	}

	SerialParser::SerialParser(IMessageHandler* pMessageHandler) :
			m_pMessageHandler(pMessageHandler)
	{
		SingletonFactory factory;
		m_pLogger = factory.retrieve<Logging::BaseLogger>();
		m_pTimebase = factory.retrieve<ITimebase>();

		if (m_pTimebase.get() == nullptr)
		{
			m_pLogger->logString(fatal, "Unable to retrieve timebase singleton");
		}

		m_candidateMessage.reserve(MAX_INCOMING_MESSAGE_LENGTH/4);
	}

	SerialParser::~SerialParser()
	{
		if (m_pMessage != nullptr)
			delete m_pMessage;
	}

	// ISR function to receive the incoming character from the UART buffer
	void SerialParser::ReceiveChar(uint8_t new_char)
	{
		m_lastCharReceivedTime = m_pTimebase->getTime_ms();

		if (m_incomingBuffer.size() <= INCOMING_BUFFER_SIZE)
			m_incomingBuffer.push(new_char);
		else
		{
			m_pLogger->logString(error, "Input serial buffer overflow, discarding buffer");

			// clear the incoming buffer queue
			m_incomingBuffer = {};
		}
	}

	void SerialParser::ProcessIncomingData()
	{
		uint8_t new_char;
		std::string logString;
		std::vector<uint8_t> stopSequenceCandidate{};
		stopSequenceCandidate.reserve(STOP_SEQUENCE_LENGTH);

		// check for timeout in transmission
		if (m_pTimebase->getTime_ms() < m_lastCharReceivedTime)
			// clock has rolled over, neglect potential timeout
			m_lastCharReceivedTime = m_pTimebase->getTime_ms();
		else if ((m_pTimebase->getTime_ms() - m_lastCharReceivedTime) >= SERIAL_TRANSMISSION_TIMEOUT_TICKS)
		{
			m_pLogger->logString(warning, "Serial packet timed out before completion, discarding");
			m_incomingBuffer = {};
			m_state = IDLE;
		}

		while (!m_incomingBuffer.empty())
		{
			new_char = m_incomingBuffer.front();
			m_incomingBuffer.pop();

			// C++ seems to be happiest when we append a char to a string using
			// the push_back method
			logString = "Received new character ";
			logString.push_back(new_char);
			m_pLogger->logString(debug, logString);

			switch(m_state)
			{
			case IDLE:
				m_startSequenceCandidate.clear();
				m_startSequenceCandidate.push_back(new_char);
				m_state = STARTING;
				break;

			case STARTING:
				m_startSequenceCandidate.push_back(new_char);
				if (m_startSequenceCandidate.size() == START_SEQUENCE_LENGTH)
				{
					if (sequenceMatch(START_SEQUENCE, m_startSequenceCandidate, START_SEQUENCE_LENGTH))
					{
						// we have matched the start sequence, move to RECEIVING state
						m_pLogger->logString(debug, std::string("Received start sequence"));
						m_state = RECEIVING;
						m_startSequenceCandidate.clear();
					}
					else
					{
						// remove first character and wait for next one to arrive
						m_startSequenceCandidate.pop_front();
					}
				}

				break;

			case RECEIVING:
				m_candidateMessage.push_back(new_char);

				if (m_candidateMessage.size() >= STOP_SEQUENCE_LENGTH)
				{
					stopSequenceCandidate.insert(stopSequenceCandidate.end(), m_candidateMessage.end()-4, m_candidateMessage.end());
					if (sequenceMatch(STOP_SEQUENCE, stopSequenceCandidate, STOP_SEQUENCE_LENGTH))
					{
						// we have received the stop sequence, message is complete
						Message msg;
						msg.loadFromBytes(m_candidateMessage, STOP_SEQUENCE_LENGTH);
						m_pMessageHandler->loadIncomingMessage(msg);

						m_pLogger->logString(debug, std::string("Received stop sequence, processing message"));

						// we are finished receiving the current message so return to IDLE
						m_state = IDLE;
						m_candidateMessage.clear();
						break;
					}
					stopSequenceCandidate.clear();
				}
				else if (m_candidateMessage.size() == MAX_INCOMING_MESSAGE_LENGTH)
				{
					// we have reached the maximum message length without hitting the stop
					// sequence; discard the message and start over
					m_state = IDLE;
					m_candidateMessage.clear();

					m_pLogger->logString(error, "Incoming message overflow");
				}

				break;

			default:
				m_pLogger->logString(error, "SerialParser in invalid state, resetting to idle");
				m_state = IDLE;
				m_candidateMessage.clear();
				break;
			}
		}
		m_incomingBuffer = {};
	}

} /* namespace MessageHandling */
