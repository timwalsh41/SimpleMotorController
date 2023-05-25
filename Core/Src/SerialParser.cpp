/*
 * SerialParser.cpp
 *
 *  Created on: May 24, 2023
 *      Author: tim
 */

#include "SerialParser.h"
#include "Message.h"

bool sequenceMatch(const char* refSequence, const char* compSequence, size_t sequenceLength)
{
	for (size_t i = 0; i < sequenceLength; i++)
		if (refSequence[i] != compSequence[i])
			return false;

	return true;
}

SerialParser::SerialParser()
{

}

SerialParser::~SerialParser()
{
	if (m_pMessage != nullptr)
		delete m_pMessage;
}

void SerialParser::ReceiveChar(uint8_t new_char)
{
	// TODO: refactor this to just add the received char to a buffer and then process messages in a tick function
	// so we're not doing all of this inside the ISR

	// TODO: add timeout if time between characters is too long, discard message sequence and start fresh;
	// to do this, just set the state to IDLE before the switch statement

	switch(m_state)
	{
	case IDLE:
		m_startSequenceCandidate[0] = new_char;
		m_startSequenceCandidateLength = 1;
		return;

	case STARTING:
		m_startSequenceCandidate[m_startSequenceCandidateLength++] = new_char;
		if (m_startSequenceCandidateLength == START_SEQUENCE_LENGTH)
		{
			if (sequenceMatch(START_SEQUENCE, (const char*) m_startSequenceCandidate, START_SEQUENCE_LENGTH))
			{
				// we have matched the start sequence, move to RECEIVING state
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

		return;

	case RECEIVING:
		m_candidateMessage[m_candidateMessageLength++] = new_char;

		if (m_candidateMessageLength >= STOP_SEQUENCE_LENGTH)
		{
			if (sequenceMatch(STOP_SEQUENCE, \
							  (const char*) &m_candidateMessage[m_candidateMessageLength - STOP_SEQUENCE_LENGTH], \
							  STOP_SEQUENCE_LENGTH))
			{
				// we have received the stop sequence, message is complete
				Message* msg = new Message;
				msg->loadFromBytes(m_candidateMessageLength - STOP_SEQUENCE_LENGTH, m_candidateMessage);

				// TODO pass message to message handler

				delete msg;

				// we are finished receiving the current message so return to IDLE
				m_state = IDLE;
				m_candidateMessageLength = 0;
				return;
			}
		}
		else if (m_candidateMessageLength == MAX_INCOMING_MESSAGE_LENGTH)
		{
			// we have reached the maximum message length without hitting the stop
			// sequence; discard the message and start over
			m_state = IDLE;
			m_candidateMessageLength = 0;

			// TODO: Log this
		}

		return;

	default:
		// TODO: log this
		return;
	}
}
