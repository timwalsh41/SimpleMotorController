/*
 * CommandInterface.h
 *
 *  Created on: May 28, 2023
 *      Author: tim
 */

#ifndef INC_COMMANDINTERFACE_H_
#define INC_COMMANDINTERFACE_H_

#include "stdint.h"

// Message start and stop sequences
constexpr char START_SEQUENCE[] = {'Q', 'R', 'S', 'T'};
constexpr char STOP_SEQUENCE[] = {'M', 'N', 'O', 'P'};

constexpr size_t START_SEQUENCE_LENGTH = sizeof(START_SEQUENCE)/sizeof(char);
constexpr size_t STOP_SEQUENCE_LENGTH = sizeof(STOP_SEQUENCE)/sizeof(char);

// bit mask for acknowledge bit in command code
constexpr uint8_t ACK_BIT_MASK = 0x80;

// Response code for invalid CRC in original message
constexpr uint8_t ACK_INVALID_CRC = 0xAA;


#endif /* INC_COMMANDINTERFACE_H_ */
