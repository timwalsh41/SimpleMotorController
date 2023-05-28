/*
 * SerialParser.h
 *
 *  Created on: May 24, 2023
 *      Author: tim
 */

#ifndef INC_SERIALPARSER_H_
#define INC_SERIALPARSER_H_

#include "stdint.h"

enum ParserState {
	IDLE,
	STARTING,
	RECEIVING
};

constexpr char START_SEQUENCE[] = {'Q', 'R', 'S', 'T'};
constexpr char STOP_SEQUENCE[] = {'M', 'N', 'O', 'P'};

constexpr size_t START_SEQUENCE_LENGTH = sizeof(START_SEQUENCE)/sizeof(char);
constexpr size_t STOP_SEQUENCE_LENGTH = sizeof(STOP_SEQUENCE)/sizeof(char);

constexpr size_t MAX_INCOMING_MESSAGE_LENGTH = 128;
constexpr size_t INCOMING_BUFFER_SIZE = 128;

#ifdef DEBUG
	constexpr uint32_t SERIAL_TRANSMISSION_TIMEOUT_TICKS = 5000;
#else
	constexpr uint32_t SERIAL_TRANSMISSION_TIMEOUT_TICKS = 50;
#endif

bool sequenceMatch(const char* ref_sequence, const char* comp_sequence, size_t sequenceLength);

class SerialParser {
public:
	SerialParser();
	~SerialParser();

	SerialParser(const SerialParser&) = delete;

	void ReceiveChar(uint8_t new_char);
	void ProcessIncomingData();
private:
	ParserState m_state{IDLE};
	uint32_t m_messageLength;
	uint8_t* m_pMessage{nullptr};

	uint8_t m_incomingBufferPtr{0};
	uint8_t m_incomingBuffer[INCOMING_BUFFER_SIZE];

	uint8_t m_startSequenceCandidate[START_SEQUENCE_LENGTH];
	uint8_t m_startSequenceCandidateLength{0};

	uint8_t m_candidateMessage[MAX_INCOMING_MESSAGE_LENGTH];
	uint32_t m_candidateMessageLength{0};

	uint32_t m_lastCharReceivedTime{0};
};



#endif /* INC_SERIALPARSER_H_ */
