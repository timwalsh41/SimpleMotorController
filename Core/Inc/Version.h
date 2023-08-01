/*
 * Version.h
 *
 *  Created on: May 30, 2023
 *      Author: tim
 */

#ifndef INC_VERSION_H_
#define INC_VERSION_H_

#include "Message.h"
#include <vector>

namespace Utility
{
	constexpr uint8_t MAJOR_VERSION = 0;
	constexpr uint8_t MINOR_VERSION = 0;
	constexpr uint8_t BUILD = 1;

	bool sendVersionMessage(const MessageHandling::Message&, std::vector<uint8_t>&);
	void getVersion(uint8_t* version);
} // namespace Utility
#endif /* INC_VERSION_H_ */
