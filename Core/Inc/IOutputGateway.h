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

	virtual void addLogString(std::string& msg) = 0;
	virtual void sendMessage(MessageHandling::Message& msg) = 0;
	virtual void processOutgoingMessages() = 0;
};

} /* namespace MessageHandling */

#endif /* INC_IOUTPUTGATEWAY_H_ */
