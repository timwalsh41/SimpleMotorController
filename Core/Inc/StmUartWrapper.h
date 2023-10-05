/*
 * StmUartWrapper.h
 *
 *  Created on: Sep 11, 2023
 *      Author: tim
 */

#ifndef INC_STMUARTWRAPPER_H_
#define INC_STMUARTWRAPPER_H_

#include "stm32l4xx_hal.h"
#include <IUartWrapper.h>
#include <SerialParser.h>
#include <memory>

class StmUartWrapper: public IUartWrapper {
public:
	StmUartWrapper(UART_HandleTypeDef* huart) : m_pUart{huart} {}
	~StmUartWrapper() {}

	void transmitString(const std::string&) const override;

	UART_HandleTypeDef* getUartPointer() { return m_pUart; }

private:
	UART_HandleTypeDef* m_pUart;
};

#endif /* INC_STMUARTWRAPPER_H_ */
