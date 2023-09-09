/*
 * Version.cpp
 *
 *  Created on: May 30, 2023
 *      Author: tim
 */

#include "Version.h"
#include "ILogger.h"
#include "SingletonFactory.h"
#include <memory>

extern std::shared_ptr<Logging::ILogger> pLogger;

namespace Utility
{

	bool sendVersionMessage(const MessageHandling::Message& msg, std::vector<uint8_t>& returnData)
	{
		SingletonFactory factory;
		std::shared_ptr<Logging::ILogger> pLogger = factory.retrieve<Logging::ILogger>();

		// check packet format
		if (msg.getPayloadLength() > 0)
		{
			pLogger->logString(error, std::string("Invalid payload in get version command"));
			return false;
		}

		returnData.push_back(MAJOR_VERSION);
		returnData.push_back(MINOR_VERSION);
		returnData.push_back(BUILD);

		pLogger->logString(info, "Successfully processed version command");

		return true;
	}

	void getVersion(uint8_t* version)
	{
		version[0] = MAJOR_VERSION;
		version[1] = MINOR_VERSION;
		version[2] = BUILD;
	}

} // namespace Utility
