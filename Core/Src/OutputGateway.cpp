/*
 * OutputGateway.cpp
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#include <cstring>
#include "stm32l4xx_hal.h"
#include "OutputGateway.h"
#include "Utility.h"

extern UART_HandleTypeDef huart2;

namespace MessageHandling
{

	OutputGateway::OutputGateway()
	{

	}

	OutputGateway::~OutputGateway()
	{

	}

	// TODO: need to handle log messages with a full output message if running in release mode
	void OutputGateway::logString(LogLevel level, const std::string& msg)
	{
		std::string outputString = "[" + std::to_string(HAL_GetTick()) + "]" + \
								std::string(logLevelString[level]) + ": " + msg + '\n';
		m_logStringQueue.push(outputString);
	}

	void OutputGateway::sendMessage(const Message& msg)
	{
		m_messageQueue.push(std::move(msg));
	}

	void OutputGateway::transmitString(const std::string& outString)
	{
		setTransmitting();
		HAL_UART_Transmit_IT(&huart2, (const uint8_t*) outString.c_str(), outString.length());

		// adding a bit of delay here seems to keep the transmitted array from being corrupted
		// TODO: could try to replace this with a shorter delay e.g. using NOP
		// HAL_Delay(1);
		Utility::delayCycles(250);
	}

	void OutputGateway::transmitMessage(const Message& msg)
	{
		uint8_t* message = nullptr;
		uint32_t length = 0;

		// get serialized message, note this allocates memory for message that
		// we are responsible to free
		if (msg.toByteArray(&message, length))
		{
			uint8_t* fullMessage = nullptr;
			uint32_t fullMessageLength = 0;

			// add the start and stop sequence to the raw serialized message, this function
			// call allocates memory for fullMessage that we are responsible to free
			addStartStopSequence(message, length, &fullMessage, fullMessageLength);
			delete[] message;

			// TODO: for debug mode, transmit a text version of the message

			setTransmitting();
			HAL_UART_Transmit_IT(&huart2, fullMessage, fullMessageLength);

			// adding a bit of delay here seems to keep the transmitted array from being corrupted
			// TODO: could try to replace this with a shorter delay
			// HAL_Delay(1);
			Utility::delayCycles(250);

			delete[] fullMessage;
		}
		else
			logString(error, "Unable to serialize and transmit output message");

		if (message != nullptr)
			delete[] message;
	}

	void OutputGateway::processOutgoingMessages()
	{
		if (!m_receiving)
			// nothing to do while we're in the middle of a transmission
			return;

		// send one message or log entry, if any exist, we only send one per call
		// to allow for transmission
		if (!m_messageQueue.empty())
		{
			transmitMessage(m_messageQueue.front());
			// TODO: make sure lifetime of the message is OK here, once we pop
			// it will get destroyed
			m_messageQueue.pop();
		}
		else if (!m_logStringQueue.empty())
		{
			transmitString(m_logStringQueue.front());
			// TODO: make sure lifetime of the string is OK here, once we
			// pop it will get destroyed
			m_logStringQueue.pop();
		}
	}

	void OutputGateway::addStartStopSequence(const uint8_t* inputArray, uint32_t inputLength, uint8_t** outputArray, uint32_t& outputLength) const
	{
		outputLength = inputLength + START_SEQUENCE_LENGTH + STOP_SEQUENCE_LENGTH;
		*outputArray = new uint8_t[outputLength];

		// The handling of the double pointer is tricky. We need to pass in outputArray as
		// a double pointer as we're doing the memory allocation here. So in the calling function
		// we create a uint8_t pointer. If we were to just pass that into the function, that would
		// just pass the value "0" (or some garbage address) since there is no memory allocated to
		// that pointer. So we pass the address of the uint8_t pointer, which is the double pointer
		// we manipulate here. So now we can allocate memory and return the address of our reserved
		// block at the address of the base pointer in the calling function.
		//
		// To copy into the outputArray, we need to get the right memory addresses within the
		// array. *outputArray is the address of the first byte of the array. So we can simply
		// add offsets to that to get the right memory address to copy the individual blocks.
		//
		// To view the data in a watch window, we need to perform pointer/array arithmetic from
		// the base address of our allocated memory block, which again is *outputArray. So, for
		// example, to see the element at location 4, we can watch (*outputArray)[4]. The
		// parentheses are important, with the parentheses we basically say go 4 bytes past
		// address *outputArray and dereference. Without the parentheses, the index operator is
		// executed first, so we actually take outputArray, add 4 to it, and then dereference that.

		std::memcpy(*outputArray, START_SEQUENCE, START_SEQUENCE_LENGTH);
		std::memcpy(*outputArray + START_SEQUENCE_LENGTH, inputArray, inputLength);
		std::memcpy(*outputArray + outputLength - STOP_SEQUENCE_LENGTH, STOP_SEQUENCE, STOP_SEQUENCE_LENGTH);
	}

} /* namespace MessageHandling */
