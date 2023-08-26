/*
 * IMessageHandler.h
 *
 *  Created on: Aug 22, 2023
 *      Author: tim
 */

#ifndef INC_IMESSAGEHANDLER_H_
#define INC_IMESSAGEHANDLER_H_

#include "Message.h"

namespace MessageHandling
{
	class IMessageHandler {
	public:
		virtual ~IMessageHandler() {}
		virtual void loadIncomingMessage(Message& msg) = 0;
		virtual void processMessages() = 0;
	};

} /* namespace MessageHandling */



#endif /* INC_IMESSAGEHANDLER_H_ */
