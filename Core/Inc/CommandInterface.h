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
constexpr char START_SEQUENCE[] = {'R', 'U', 'S', 'T'};
constexpr char STOP_SEQUENCE[] = {'S', 'T', 'O', 'P'};

constexpr size_t START_SEQUENCE_LENGTH = sizeof(START_SEQUENCE)/sizeof(char);
constexpr size_t STOP_SEQUENCE_LENGTH = sizeof(STOP_SEQUENCE)/sizeof(char);

// Response code for command processed successfully
constexpr uint8_t ACK_SUCCESSFUL = 0x2B; // '+'

// Response code for invalid CRC in original message
constexpr uint8_t ACK_INVALID_CRC = 0x28;	// '('

// Response code for invalid command code
constexpr uint8_t ACK_INVALID_COMMAND = 0x29; 	// ')'

// Response code for command processing error
constexpr uint8_t ACK_COMMAND_ERROR = 0x2A; 	// '*'

// Commands
constexpr uint8_t CMD_ENABLE_MOTOR = 0x30;		// '0'
constexpr uint8_t CMD_DISABLE_MOTOR = 0x31;		// '1'
constexpr uint8_t CMD_SLEEP_DRIVER = 0x32;		// '2'
constexpr uint8_t CMD_WAKE_DRIVER = 0x33;		// '3'
constexpr uint8_t CMD_CLEAR_FAULT = 0x34;		// '3'
constexpr uint8_t CMD_GET_MOTOR_POS = 0x40;		// '@'
constexpr uint8_t CMD_GET_MOTOR_PWM = 0x41;		// 'A'
constexpr uint8_t CMD_GET_MOTOR_SPEED = 0x42;	// 'B'
constexpr uint8_t CMD_GET_MOTOR_DIR = 0x43;		// 'C'
constexpr uint8_t CMD_GET_MOTOR_CURRENT = 0x44;	// 'D'
constexpr uint8_t CMD_RAMP_MOTOR_ON = 0x50;		// 'P'
constexpr uint8_t CMD_RAMP_MOTOR_OFF = 0x51;	// 'Q'
constexpr uint8_t CMD_SET_MAX_DC = 0x52;		// 'R'
constexpr uint8_t CMD_INJECT_FAULT = 0x60;		// '`'
constexpr uint8_t CMD_GET_VERSION = 0x70;		// 'p'
constexpr uint8_t CMD_GET_STATUS = 0x71;		// 'q'
constexpr uint8_t CMD_GET_LOG_MESSAGE = 0x72;	// 'r'

#endif /* INC_COMMANDINTERFACE_H_ */
