/*
 * message.h
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#ifndef INC_MESSAGE_H_
#define INC_MESSAGE_H_

#include "stdint.h"
#include <cstddef>  // required for gtest build to find size_t
#include <vector>
#include <memory>

namespace Logging
{
	// forward definition of BaseLogger class
	class BaseLogger;
}

namespace MessageHandling
{
	// CRC length is 16 bits or 2 bytes
	constexpr uint8_t CRC_LENGTH = 2;

	// minimum payload length is 3 bytes (one byte command code plus two byte CRC)
	constexpr uint8_t MINIMUM_MESSAGE_LENGTH = 1 + CRC_LENGTH;

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

		bool loadFromBytes(const std::vector<uint8_t>& raw_message, const size_t ignoreLength);
		bool checkCRC() const;
		void setCRC();
		uint8_t getCommandCode() const { return m_commandCode; }
		uint32_t getPayloadLength() const { return m_payload.size(); }
		uint16_t getCRC() const { return m_crc; };

		void setCommandCode(uint8_t commandCode) { m_commandCode = commandCode; }
		void setAck() { m_commandCode |= ACK_BIT_MASK; }
		void clearAck() { m_commandCode &= ~ACK_BIT_MASK; }
		void setPayload(const std::vector<uint8_t>& payload);
		void setPayload(const uint8_t&);
		void setPayload(std::vector<uint8_t>&& payload);

		const std::vector<uint8_t>& getPayload() const { return m_payload; }

		bool toByteArray(std::vector<uint8_t>& message) const;

	private:
		uint16_t calculateMessageCRC() const;

		uint8_t m_commandCode{0};
		std::vector<uint8_t> m_payload;
		uint16_t m_crc{0};
		std::shared_ptr<Logging::BaseLogger> m_pLogger;
	};

} /* namespace Message */
#endif /* INC_MESSAGE_H_ */
