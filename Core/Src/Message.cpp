/*
 * message.cpp
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#include <Message.h>
#include <cstring>

Message::Message()
{

}

Message::~Message()
{
	if (m_pPayload != nullptr)
		delete m_pPayload;
}

bool Message::loadFromBytes(uint8_t message_length, const uint8_t* const raw_message)
{
	if (message_length < MINIMUM_MESSAGE_LENGTH)
	{
		// TODO: add logging
		return false;
	}

	m_commandCode = raw_message[0];
	m_payloadLength = message_length - 3;

	if (m_payloadLength)
	{
		m_pPayload = new uint8_t[m_payloadLength];
		std::memcpy(m_pPayload, &raw_message[1], m_payloadLength);
	}

	std::memcpy(m_crc, &raw_message[message_length-2], 2);
	return true;
}

bool Message::crcCheck(uint8_t payload_length, const uint8_t* const raw_message)
{
	// TODO: CRC calculation
	return true;
}
