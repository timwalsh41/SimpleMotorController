/*
 * message.cpp
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#include <cstring>
#include "Message.h"
#include "Logger.h"

extern Logging::Logger theLogger;

// TODO: Add message ID and timestamp field

namespace MessageHandling
{

	Message::Message()
	{

	}

	Message::Message(const Message& other) :
			m_commandCode{other.m_commandCode},
			m_crc{other.m_crc}
	{
		if (!other.m_payload.empty())
		{
			m_payload = other.m_payload;
		}
	}

	Message::Message(Message&& other) noexcept :
			m_commandCode{other.m_commandCode},
			m_crc{other.m_crc}
	{
		// theLogger.logString(debug, "Move construction of Message");

		if (!other.m_payload.empty())
		{
			m_payload = std::move(other.m_payload);
		}
	}

	Message::~Message()
	{

	}

	bool Message::loadFromBytes(uint8_t message_length, const uint8_t* const raw_message)
	{
		uint8_t payloadLength;
		m_payload.clear();

		if (message_length < MINIMUM_MESSAGE_LENGTH)
		{
			theLogger.logString(warning, "Received message less than the minimum length");
			return false;
		}

		m_commandCode = raw_message[0];
		payloadLength = message_length - 3;

		if (payloadLength)
		{
			m_payload.reserve(payloadLength);
			m_payload.insert(m_payload.begin(), &raw_message[1], &raw_message[1+payloadLength]);
		}

		// TODO: make sure endianness is correct here
		std::memcpy(&m_crc, &raw_message[message_length-2], 2);
		return true;
	}

	uint16_t Message::calculateMessageCRC() const
	{
#ifdef DEBUG
		// return "ab" as the CRC16, in little endian format (least significant byte at lowest
		// memory address
		return 0x6261;
#else
		// Calculate CRC16 for the input byte array, set the two byte response
		// in the provided crc pointer
		// Uses CRC-CCITT polynomial (0x1021)
		// Reference:
		// https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Bit_ordering_(endianness)
		// Useful CRC calculator to validate code:
		// http://www.zorc.breitbandkatze.de/crc.html

		uint16_t rem = 0;

		auto processWord = [](uint16_t rem, uint8_t byte)
		{
			rem ^= static_cast<uint16_t>(byte) << 8;

			for (size_t i = 0; i < 8; i++)
			{
				if (rem & 0x8000)
					rem = (rem << 1) ^ 0x1021;
				else
					rem = rem << 1;
			}

			return rem;
		};

		rem = processWord(rem, m_commandCode);
		for (uint8_t byte : m_payload)
		{
			rem = processWord(rem, byte);
		}
		return rem;
#endif
	}

	// Check the CRC, used for validating an incoming message
	// CRC is calculated on the command byte plus payload
	bool Message::checkCRC() const
	{
		return calculateMessageCRC() == m_crc;
	}

	// Set the CRC, used for stamping an outgoing message
	void Message::setCRC()
	{
		m_crc = calculateMessageCRC();
	}

	void Message::setPayload(const std::vector<uint8_t>& payload)
	{
		if (!payload.empty())
			m_payload = payload;
	}

	void Message::setPayload(const uint8_t& payload)
	{
		m_payload.clear();
		m_payload.push_back(payload);
	}

	Message& Message::operator=(const Message& other)
	{
		if (&other == this)
			return *this;

		m_commandCode = other.m_commandCode;
		m_crc = other.m_crc;

		if (!other.m_payload.empty())
		{
			m_payload = other.m_payload;
		}

		return *this;
	}

	Message& Message::operator=(Message&& other) noexcept
	{
		if (&other == this)
			return *this;

		// theLogger.logString(debug, std::string("Move assignment of Message"));

		m_commandCode = other.m_commandCode;
		m_crc = other.m_crc;

		if (!other.m_payload.empty())
		{
			m_payload = std::move(other.m_payload);
		}

		return *this;
	}

	// Convert the message to a byte array, this will allocate memory for the byte array and assign
	// to the *message argument; the caller is responsible to free the memory
	// This serialization includes the command code, payload, and crc; for transmission the start and
	// end sequences should be added by the caller
	bool Message::toByteArray(uint8_t **message, uint32_t& length) const
	{
		if (!checkCRC())
		{
			length = 0;
			theLogger.logString(error, "Unable to serialize message, CRC mismatch");
			return false;
		}

		// total length includes command code, payload, and 2 byte CRC
		length = 3 + m_payload.size();

		*message = new uint8_t[length];

		if (message == nullptr)
		{
			theLogger.logString(error, "Unable to serialize message, cannot allocate memory for byte array");
			return false;
		}

		// the double pointer management here is tricky, see extensive notes in
		// OutputGateway::addStartStopSequence

		*message[0] = m_commandCode;
		if (!m_payload.empty())
			std::memcpy(*message+1, m_payload.data(), m_payload.size());

		// note this stores in little endian format (least significant byte at the lowest
		// memory address)
		std::memcpy(*message + length - 2, &m_crc, 2);

		return true;
	}

} /* namespace Message */
