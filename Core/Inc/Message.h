/*
 * message.h
 *
 *  Created on: Apr 30, 2023
 *      Author: tim
 */

#ifndef INC_MESSAGE_H_
#define INC_MESSAGE_H_

#include "stdint.h"

// minimum payload length is 3 bytes (one byte command code plus two byte CRC)
constexpr uint8_t MINIMUM_MESSAGE_LENGTH = 3;

class Message
{
public:
	Message();
	~Message();

	Message(Message& other) = delete;

	bool loadFromBytes(uint8_t payload_length, const uint8_t* const raw_message);
	uint8_t getCommandCode() { return m_commandCode; }
	uint32_t getPayloadLength() { return m_payloadLength; }
	uint8_t* getPayload() { return m_pPayload; }


private:
	uint8_t m_commandCode{0};
	uint32_t m_payloadLength{0};
	uint8_t* m_pPayload{nullptr};
	uint8_t m_crc[2]{0,0};

	bool crcCheck(uint8_t payload_length, const uint8_t* const raw_message);
};


#endif /* INC_MESSAGE_H_ */
