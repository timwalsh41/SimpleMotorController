/*
 * IUartWrapper.h
 *
 *  Created on: Sep 11, 2023
 *      Author: tim
 */

#ifndef INC_IUARTWRAPPER_H_
#define INC_IUARTWRAPPER_H_

#include <string>
#include <vector>
#include <functional>

class IUartWrapper
{
public:
	virtual ~IUartWrapper() {}
	virtual void transmitString(std::string&) = 0;
};

#endif /* INC_IUARTWRAPPER_H_ */
