/*
 * Version.cpp
 *
 *  Created on: May 30, 2023
 *      Author: tim
 */

#include "Version.h"
#include "OutputGateway.h"
#include "Logger.h"

extern MessageHandling::OutputGateway theOutputGateway;
extern Logging::Logger theLogger;

namespace Utility
{

	bool sendVersionMessage(const MessageHandling::Message& msg, uint8_t* returnData, uint8_t& returnDataLength)
	{


		// check packet format
		if (msg.getPayloadLength() > 0)
		{
			theLogger.logString(error, std::string("Invalid payload in get version command"));
			return false;
		}

		returnData = new uint8_t[3];
		returnData[0] = MAJOR_VERSION;
		returnData[1] = MINOR_VERSION;
		returnData[2] = BUILD;

		theLogger.logString(info, "Successfully processed version command");

		return true;
	}

	void getVersion(uint8_t* version)
	{
		version[0] = MAJOR_VERSION;
		version[1] = MINOR_VERSION;
		version[2] = BUILD;
	}

} // namespace Utility
