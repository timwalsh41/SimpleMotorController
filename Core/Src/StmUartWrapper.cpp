/*
 * StmUartWrapper.cpp
 *
 *  Created on: Sep 11, 2023
 *      Author: tim
 */

#include <StmUartWrapper.h>

// required global variable for setting UART back to receive mode
extern uint8_t UartRcvBuffer;

void StmUartWrapper::transmitString(const std::string& outString) const
{
	HAL_UART_Transmit_IT(m_pUart, (const uint8_t*) outString.c_str(), outString.length());
}

void StmUartWrapper::registerUserTxCallback(const std::function<void()> txCallback)
{
	m_userTxCallback = txCallback;
}

void StmUartWrapper::registerUserRxCallback(const std::function<void()> rxCallback)
{
	m_userRxCallback = rxCallback;
}

void StmUartWrapper::TxCallback(UART_HandleTypeDef* huart)
{
	if (m_userTxCallback != nullptr)
		m_userTxCallback();

	HAL_UART_Receive_IT(m_pUart, &UartRcvBuffer, 1);
}

void StmUartWrapper::RxCallback(UART_HandleTypeDef* huart)
{
	if (m_userRxCallback != nullptr)
		m_userRxCallback();
}

