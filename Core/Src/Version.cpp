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

	bool sendVersionMessage(const MessageHandling::Message& msg)
	{
		MessageHandling::Message output;
		output.setCommandCode(msg.getCommandCode());
		output.setAck();

		// check packet format
		if (msg.getPayloadLength() > 0)
		{
			uint8_t payload = ACK_INVALID_PAYLOAD;
			output.setPayload(&payload, 1);
			output.setCRC();
			theOutputGateway.sendMessage(output);

			theLogger.logString(error, std::string("Invalid payload in get version command"));
			return false;
		}

		uint8_t version[3] = {MAJOR_VERSION, MINOR_VERSION, BUILD};
		output.setPayload(version, 3);
		output.setCRC();
		theOutputGateway.sendMessage(output);
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
