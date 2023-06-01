/*
 * CommandInterface.h
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#ifndef INC_COMMANDINTERFACE_H_
#define INC_COMMANDINTERFACE_H_

// include cstring for size_t
#include <cstring>
#include "stdint.h"

// Message start and stop sequences
constexpr char START_SEQUENCE[] = {'Q', 'R', 'S', 'T'};
constexpr char STOP_SEQUENCE[] = {'M', 'N', 'O', 'P'};

constexpr size_t START_SEQUENCE_LENGTH = sizeof(START_SEQUENCE)/sizeof(char);
constexpr size_t STOP_SEQUENCE_LENGTH = sizeof(STOP_SEQUENCE)/sizeof(char);

// Response code for invalid CRC in original message
constexpr uint8_t ACK_INVALID_CRC = 0x28;	// '('

// Response code for invalid command code
constexpr uint8_t ACK_INVALID_COMMAND = 0x29; 	// ')'

// Response code for invalid payload
constexpr uint8_t ACK_INVALID_PAYLOAD = 0x2A; 	// '*'

// Response code for invalid message format
constexpr uint8_t ACK_INVALID_FORMAT = 0x2B;	// '+'

constexpr uint8_t CMD_GET_VERSION = 0x70;	// 'p'

#endif /* INC_COMMANDINTERFACE_H_ */
