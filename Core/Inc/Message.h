/*
 * message.h
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#ifndef INC_MESSAGE_H_
#define INC_MESSAGE_H_

#include "stdint.h"

namespace MessageHandling
{

	// minimum payload length is 3 bytes (one byte command code plus two byte CRC)
	constexpr uint8_t MINIMUM_MESSAGE_LENGTH = 3;

	// bit mask for acknowledge bit in command code
	constexpr uint8_t ACK_BIT_MASK = 0x80;

	uint16_t calculateCRC(uint8_t* byteArray, uint32_t length);

	class Message
	{
	public:
		Message();
		~Message();
		Message(const Message&);
		Message(Message&&) noexcept;

		Message& operator=(const Message& other);
		Message& operator=(Message&& other) noexcept;

		bool loadFromBytes(uint8_t payload_length, const uint8_t* const raw_message);
		bool checkCRC() const;
		void setCRC();
		uint8_t getCommandCode() const { return m_commandCode; }
		uint32_t getPayloadLength() const { return m_payloadLength; }
		uint8_t* getPayload() const { return m_pPayload; }
		uint16_t getCRC() const { return m_crc; };

		void setCommandCode(uint8_t commandCode) { m_commandCode = commandCode; }
		void setAck() { m_commandCode |= ACK_BIT_MASK; }
		void clearAck() { m_commandCode &= ~ACK_BIT_MASK; }
		void setPayload(const uint8_t* payload, uint32_t payloadLength);

		bool toByteArray(uint8_t **message, uint32_t& length) const;

	private:
		uint16_t calculateMessageCRC() const;

		uint8_t m_commandCode{0};
		uint32_t m_payloadLength{0};
		uint8_t* m_pPayload{nullptr};
		uint16_t m_crc{0};
	};

} /* namespace Message */
#endif /* INC_MESSAGE_H_ */
