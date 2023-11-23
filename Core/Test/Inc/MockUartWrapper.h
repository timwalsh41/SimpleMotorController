/*
 * MockUartWrapper.h
 *
 *  Created on: Sep 26, 2023
 *      Author: tim
 */

#ifndef INC_MOCKUARTWRAPPER_H_
#define INC_MOCKUARTWRAPPER_H_

#include <IUartWrapper.h>

class MockUartWrapper : public IUartWrapper
{
public:
	MOCK_METHOD(void, transmitString, (std::string&), (override));
};

#endif /* INC_MOCKUARTGATEWAY_H_ */
