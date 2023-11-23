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
#include "BaseLogger.h"
#include <queue>
#include <deque>
#include <vector>
#include <memory>

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

	class SerialParser {
	public:
		SerialParser(IMessageHandler*);
		~SerialParser();

		void ReceiveChar(uint8_t new_char);
		void ReceiveChars(std::vector<uint8_t>& new_chars);
		void ProcessIncomingData();
	private:
		template <typename T>
		bool sequenceMatch(const char* refSequence, T& compSequence, size_t refSequenceLength);

		ParserState m_state{IDLE};
		std::shared_ptr<ITimebase> m_pTimebase;
		IMessageHandler* m_pMessageHandler{nullptr};
		std::shared_ptr<Logging::BaseLogger> m_pLogger;

		uint32_t m_messageLength;
		uint8_t* m_pMessage{nullptr};

		std::queue<uint8_t> m_incomingBuffer{};
		std::deque<uint8_t> m_startSequenceCandidate{};
		std::vector<uint8_t> m_candidateMessage{};

		uint32_t m_lastCharReceivedTime{0};
	};

} /* namespace MessageHandling */
#endif /* INC_SERIALPARSER_H_ */
