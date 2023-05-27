/*
 * OutputGateway.cpp
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#include "stm32l4xx_hal.h"
#include "OutputGateway.h"

extern UART_HandleTypeDef huart2;

void OutputGateway::logString(LogLevel level, const std::string& msg)
{
	while (!m_receiving)
	{
		__NOP();
	}

	std::string outputString = "[" + std::to_string(HAL_GetTick()) + "]" + \
			std::string(logLevelString[level]) + ": " + msg + '\n';
	sendStringOutput(outputString);
}

void OutputGateway::sendStringOutput(const std::string& outString)
{
	setTransmitting();
	HAL_UART_Transmit_IT(&huart2, (const uint8_t*) outString.c_str(), outString.length());
}
