/*
 * StmUartWrapper.cpp
 *
 *  Created on: Sep 11, 2023
 *      Author: tim
 */

#include <StmUartWrapper.h>
#include "main.h"

void StmUartWrapper::transmitString(std::string& outString)
{
	// we move the string to be written into this local variable to preserve the memory until the
	// UART write is complete
	m_stringBuffer = std::move(outString);
	HAL_StatusTypeDef ret = HAL_UART_Transmit_IT(m_pUart, (const uint8_t*) m_stringBuffer.c_str(), m_stringBuffer.length());

	if (HAL_OK != ret)
	{
		Error_Handler();
	}
}
