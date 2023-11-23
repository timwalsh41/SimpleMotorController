/*
 * IOutputGateway.h
 *
 *  Created on: Sep 26, 2023
 *      Author: tim
 */

#ifndef INC_IOUTPUTGATEWAY_H_
#define INC_IOUTPUTGATEWAY_H_

#include "Message.h"
#include <string>

namespace MessageHandling
{

class IOutputGateway
{
public:
	virtual ~IOutputGateway() {}

	virtual void sendMessage(MessageHandling::Message& msg) = 0;
	virtual void processMessages() = 0;
};

} /* namespace MessageHandling */

#endif /* INC_IOUTPUTGATEWAY_H_ */
