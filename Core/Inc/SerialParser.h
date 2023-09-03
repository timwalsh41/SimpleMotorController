/*
 * SerialParser.h
 *
 *  Created on: May 24, 2023
 *      Author: tim
 */

#ifndef INC_SERIALPARSER_H_
#define INC_SERIALPARSER_H_

#include "stdint.h"
#include "CommandInterface.h"
#include "ITimebase.h"
#include "IMessageHandler.h"
#include <queue>
#include <deque>
#include <vector>

namespace MessageHandling
{

	enum ParserState {
		IDLE,
		STARTING,
		RECEIVING
	};

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
		SerialParser(ITimebase* timebase, IMessageHandler* messageHandler);
		~SerialParser();

		SerialParser(const SerialParser&) = delete;

		void ReceiveChar(uint8_t new_char);
		void ProcessIncomingData();
	private:
		ParserState m_state{IDLE};
		ITimebase* m_timebase{nullptr};
		IMessageHandler* m_messageHandler{nullptr};

		uint32_t m_messageLength;
		uint8_t* m_pMessage{nullptr};

		std::queue<uint8_t> m_incomingBuffer{};
		std::deque<uint8_t> m_startSequenceCandidate{};
		std::vector<uint8_t> m_candidateMessage{};

		uint32_t m_lastCharReceivedTime{0};
	};

} /* namespace MessageHandling */
#endif /* INC_SERIALPARSER_H_ */
