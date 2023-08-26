/*
 * SerialParser.cpp
 *
 *  Created on: May 24, 2023
 *      Author: tim
 */

#include "SerialParser.h"
#include "Message.h"
#include "MessageHandler.h"
#include "Logger.h"

extern Logging::Logger theLogger;

namespace MessageHandling
{

	bool sequenceMatch(const char* refSequence, const char* compSequence, size_t sequenceLength)
	{
		for (size_t i = 0; i < sequenceLength; i++)
			if (refSequence[i] != compSequence[i])
				return false;

		return true;
	}

	SerialParser::SerialParser(ITimebase* timebase, IMessageHandler* messageHandler) :
			m_timebase(timebase),
			m_messageHandler(messageHandler)
	{

	}

	SerialParser::~SerialParser()
	{
		if (m_pMessage != nullptr)
			delete m_pMessage;
	}

	// ISR function to receive the incoming character from the UART buffer
	void SerialParser::ReceiveChar(uint8_t new_char)
	{
		m_lastCharReceivedTime = m_timebase->getTime_ms();

		if (m_incomingBufferPtr < INCOMING_BUFFER_SIZE)
			m_incomingBuffer[m_incomingBufferPtr++] = new_char;
		else
		{
			theLogger.logString(error, "Input serial buffer overflow, discarding buffer");
			m_incomingBufferPtr = 0;
		}
	}

	void SerialParser::ProcessIncomingData()
	{
		uint8_t new_char;
		std::string logString;

		if (m_state != IDLE)
		{
			// check for timeout in transmission
			if (m_timebase->getTime_ms() < m_lastCharReceivedTime)
				// clock has rolled over, neglect potential timeout
				m_lastCharReceivedTime = m_timebase->getTime_ms();
			else if ((m_timebase->getTime_ms() - m_lastCharReceivedTime) >= SERIAL_TRANSMISSION_TIMEOUT_TICKS)
			{
				theLogger.logString(warning, "Serial packet timed out before completion, discarding");
				m_state = IDLE;
			}
		}

		for (size_t bufferIdx = 0; bufferIdx < m_incomingBufferPtr; bufferIdx++)
		{
			new_char = m_incomingBuffer[bufferIdx];

			// C++ seems to be happiest when we append a char to a string using
			// the push_back method
			logString = "Received new character ";
			logString.push_back(new_char);
			theLogger.logString(debug, logString);

			switch(m_state)
			{
			case IDLE:
				m_startSequenceCandidate[0] = new_char;
				m_startSequenceCandidateLength = 1;
				m_state = STARTING;
				break;

			case STARTING:
				m_startSequenceCandidate[m_startSequenceCandidateLength++] = new_char;
				if (m_startSequenceCandidateLength == START_SEQUENCE_LENGTH)
				{
					if (sequenceMatch(START_SEQUENCE, (const char*) m_startSequenceCandidate, START_SEQUENCE_LENGTH))
					{
						// we have matched the start sequence, move to RECEIVING state
						theLogger.logString(debug, std::string("Received start sequence"));
						m_state = RECEIVING;
						m_startSequenceCandidateLength = 0;
					}
					else
					{
						// shift the rightmost N-1 characters to the left
						for (size_t i = 0; i < START_SEQUENCE_LENGTH-1; i++)
							m_startSequenceCandidate[i] = m_startSequenceCandidate[i+1];

						--m_startSequenceCandidateLength;
					}
				}

				break;

			case RECEIVING:
				m_candidateMessage[m_candidateMessageLength++] = new_char;

				if (m_candidateMessageLength >= STOP_SEQUENCE_LENGTH)
				{
					if (sequenceMatch(STOP_SEQUENCE, \
									  (const char*) &m_candidateMessage[m_candidateMessageLength - STOP_SEQUENCE_LENGTH], \
									  STOP_SEQUENCE_LENGTH))
					{
						// we have received the stop sequence, message is complete
						Message msg;
						msg.loadFromBytes(m_candidateMessageLength - STOP_SEQUENCE_LENGTH, m_candidateMessage);
						m_messageHandler->loadIncomingMessage(msg);

						theLogger.logString(debug, std::string("Received stop sequence, processing message"));

						// we are finished receiving the current message so return to IDLE
						m_state = IDLE;
						m_candidateMessageLength = 0;
						break;
					}
				}
				else if (m_candidateMessageLength == MAX_INCOMING_MESSAGE_LENGTH)
				{
					// we have reached the maximum message length without hitting the stop
					// sequence; discard the message and start over
					m_state = IDLE;
					m_candidateMessageLength = 0;

					theLogger.logString(error, "Incoming message overflow");
				}

				break;

			default:
				theLogger.logString(error, "SerialParser in invalid state, resetting to idle");
				m_state = IDLE;
				m_candidateMessageLength = 0;
				break;
			}
		}

		m_incomingBufferPtr = 0;
	}

} /* namespace MessageHandling */
