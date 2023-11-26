/*
 * message.cpp
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */


#include "Message.h"
#include "BaseLogger.h"
#include "SingletonFactory.h"
#include <cstring>
#include <memory>

// TODO: Add message ID and timestamp field

namespace MessageHandling
{

	Message::Message()
	{
		SingletonFactory factory;
		m_pLogger = factory.retrieve<Logging::BaseLogger>();
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
		// m_pLogger->logString(debug, "Move construction of Message");

		if (!other.m_payload.empty())
		{
			m_payload = std::move(other.m_payload);
		}
	}

	Message::~Message()
	{

	}

	// extract message from raw bytes from serial parser; note these raw bytes may include the stop
	// sequence, which we'll extract by dropping the last ignoreLength bytes from the raw message
	bool Message::loadFromBytes(const std::vector<uint8_t>& raw_message, const size_t ignoreLength)
	{
		int payloadLength;
		m_payload.clear();

		if (raw_message.size() < MINIMUM_MESSAGE_LENGTH + ignoreLength)
		{
			m_pLogger->logString(warning, "Received message less than the minimum length");
			return false;
		}

		m_commandCode = raw_message[0];

		// minimum message length includes the command code plus the CRC, so the payload length is just the
		// total message size minus (command code length + payload length + ignore length)
		payloadLength = raw_message.size() - MINIMUM_MESSAGE_LENGTH - ignoreLength;

		if (payloadLength > 0)
		{
			m_payload.reserve(payloadLength);
			// copy in payload, which runs from the second byte (the one after the command code) to the end minus
			// the ignore length (stop sequence length) plus minimum message length (CRC + command code) minus one
			// byte for command code
			m_payload.insert(m_payload.begin(), raw_message.begin() + 1, raw_message.end() - ignoreLength - (MINIMUM_MESSAGE_LENGTH - 1));
		}

		// TODO: make sure endianness is correct here
		// Now grab the CRCC
		std::memcpy(&m_crc, &raw_message.data()[raw_message.size() - ignoreLength - CRC_LENGTH], CRC_LENGTH);
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

		uint16_t rem = 0xFFFF;

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

	void Message::setPayload(std::vector<uint8_t>&& payload)
	{
		if (!payload.empty())
			m_payload = std::move(payload);
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

		// m_pLogger->logString(debug, std::string("Move assignment of Message"));

		m_commandCode = other.m_commandCode;
		m_crc = other.m_crc;

		if (!other.m_payload.empty())
		{
			m_payload = std::move(other.m_payload);
		}

		return *this;
	}

	// Convert the message to a byte array. This serialization includes the command code,
	// payload, and crc; for transmission the start and end sequences should be added by
	// the caller
	bool Message::toByteArray(std::vector<uint8_t>& message) const
	{
		if (!checkCRC())
		{
			m_pLogger->logString(error, "Unable to serialize message, CRC mismatch");
			return false;
		}

		// total length includes header (command code, CRC, etc.) plus payload length
		uint32_t arrayLength = MINIMUM_MESSAGE_LENGTH + m_payload.size();

		message.reserve(arrayLength);

		if (message.capacity() != arrayLength)
		{
			m_pLogger->logString(error, "Unable to serialize message, cannot allocate memory for byte array");
			return false;
		}

		message.push_back(m_commandCode);
		if (!m_payload.empty())
			message.insert(message.end(), m_payload.begin(), m_payload.end());

		message.push_back(static_cast<uint8_t>((m_crc >> 0) & 0xFF));
		message.push_back(static_cast<uint8_t>((m_crc >> 8) & 0xFF));

		return true;
	}

} /* namespace Message */
