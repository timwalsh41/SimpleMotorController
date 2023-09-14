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
#include <ILogger.h>
#include <SerialParser.h>
#include <memory>

class StmUartWrapper: public IUartWrapper {
public:
	StmUartWrapper(UART_HandleTypeDef* huart) : m_pUart{huart} {}
	~StmUartWrapper() {}

	//StmUartWrapper(const StmUartWrapper&) = delete;
	//StmUartWrapper& operator=(const StmUartWrapper&) = delete;

	void transmitString(const std::string&) const override;

	// auxiliary functions that are
	void registerUserTxCallback(const std::function<void()>) override;
	void registerUserRxCallback(const std::function<void()>) override;

	// the actual callback functions that are registered as the UART event callbacks
	// (these call the user registered callbacks, along with any STM32 specific tasks)
	void TxCallback(UART_HandleTypeDef*);
	void RxCallback(UART_HandleTypeDef*);

	UART_HandleTypeDef* getUartPointer() { return m_pUart; }

private:
	UART_HandleTypeDef* m_pUart;
	std::function<void()> m_userTxCallback{nullptr};
	std::function<void()> m_userRxCallback{nullptr};
};

#endif /* INC_STMUARTWRAPPER_H_ */
