/*
 * message.cpp
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#include <cstring>
#include "Message.h"
#include "OutputGateway.h"

extern OutputGateway theOutputGateway;

Message::Message()
{

}

Message::Message(const Message& other) :
		m_commandCode{other.m_commandCode},
		m_payloadLength{other.m_payloadLength},
		m_crc{other.m_crc}
{
	if (m_payloadLength)
	{
		m_pPayload = new uint8_t[m_payloadLength];
		std::memcpy(m_pPayload, other.m_pPayload, m_payloadLength);
	}
}

Message::Message(Message&& other) :
		m_commandCode{other.m_commandCode},
		m_payloadLength{other.m_payloadLength},
		m_crc{other.m_crc}
{
	// theOutputGateway.logString(debug, "Move construction of Message");

	if (m_payloadLength)
	{
		m_pPayload = other.m_pPayload;
		other.m_pPayload = nullptr;
		other.m_payloadLength = 0;
	}
}

Message::~Message()
{
	if (m_pPayload != nullptr)
		delete[] m_pPayload;
}

bool Message::loadFromBytes(uint8_t message_length, const uint8_t* const raw_message)
{
	if (message_length < MINIMUM_MESSAGE_LENGTH)
	{
		theOutputGateway.logString(warning, "Received message less than the minimum length");
		return false;
	}

	m_commandCode = raw_message[0];
	m_payloadLength = message_length - 3;

	if (m_payloadLength)
	{
		m_pPayload = new uint8_t[m_payloadLength];
		std::memcpy(m_pPayload, &raw_message[1], m_payloadLength);
	}

	// TODO: make sure endianness is correct here
	std::memcpy(&m_crc, &raw_message[message_length-2], 2);
	return true;
}


#ifdef DEBUG
// Dummy CRC calculator, always return 0x6161 = 'aa'
uint16_t calculateCRC(uint8_t*, uint32_t)
{
	// return "ab" as the CRC16
	return 0x6162;
}
#else
// Calculate CRC16 for the input byte array, set the two byte response
// in the provided crc pointer
// Uses CRC-CCITT polynomial (0x1021)
// Reference:
// https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Bit_ordering_(endianness)
// Useful CRC calculator to validate code:
// http://www.zorc.breitbandkatze.de/crc.html
uint16_t calculateCRC(uint8_t* byteArray, uint32_t length)
{
	uint16_t rem = 0;

	for (size_t i = 0; i < length; i++)
	{
		rem ^= static_cast<uint16_t>(byteArray[i]) << 8;

		for (size_t j = 0; j < 8; j++)
		{
			if (rem & 0x8000)
				rem = (rem << 1) ^ 0x1021;
			else
				rem = rem << 1;
		}
	}

	return rem;
}
#endif

uint16_t Message::calculateMessageCRC() const
{
	uint8_t* byteArray = new uint8_t[m_payloadLength + 1];
	uint16_t crc;

	byteArray[0] = m_commandCode;
	if (m_payloadLength)
		std::memcpy(&byteArray[1], m_pPayload, m_payloadLength);

	crc = calculateCRC(byteArray, m_payloadLength + 1);
	delete[] byteArray;
	return crc;
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

void Message::setPayload(const uint8_t* payload, uint32_t payloadLength)
{
	m_payloadLength = payloadLength;
	m_pPayload = new uint8_t[m_payloadLength];
	std::memcpy(m_pPayload, payload, m_payloadLength);
}

Message& Message::operator=(const Message& other)
{
	if (&other == this)
		return *this;

	if (m_pPayload != nullptr)
		delete[] m_pPayload;

	m_commandCode = other.m_commandCode;
	m_payloadLength = other.m_payloadLength;
	m_crc = other.m_crc;

	if (m_payloadLength)
	{
		m_pPayload = new uint8_t[m_payloadLength];
		std::memcpy(m_pPayload, other.m_pPayload, m_payloadLength);
	}
	else
		m_pPayload = nullptr;

	return *this;
}

Message& Message::operator=(Message&& other)
{
	if (&other == this)
		return *this;

	if (m_pPayload != nullptr)
			delete[] m_pPayload;

	// theOutputGateway.logString(debug, std::string("Move assignment of Message"));

	m_commandCode = other.m_commandCode;
	m_payloadLength = other.m_payloadLength;
	m_crc = other.m_crc;

	if (m_payloadLength)
	{
		m_pPayload = other.m_pPayload;
		other.m_pPayload = nullptr;
	}
	else
		m_pPayload = nullptr;

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
		theOutputGateway.logString(error, "Unable to serialize message, CRC mismatch");
		return false;
	}

	// total length includes command code, payload, and 2 byte CRC
	length = 3 + m_payloadLength;

	*message = new uint8_t[length];

	if (message == nullptr)
	{
		theOutputGateway.logString(error, "Unable to serialize message, cannot allocate memory for byte array");
		return false;
	}

	// the double pointer management here is tricky, see extensive notes in
	// OutputGateway::addStartStopSequence

	*message[0] = m_commandCode;
	if (m_payloadLength)
		std::memcpy(*message+1, m_pPayload, m_payloadLength);

	// note this stores in little endian format (least significant byte at the lowest
	// memory address)
	std::memcpy(*message + length - 2, &m_crc, 2);

	return true;
}
