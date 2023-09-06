/*
 * Logger.h
 *
 *  Created on: Jul 14, 2023
 *      Author: tim
 */

#ifndef SRC_LOGGER_H_
#define SRC_LOGGER_H_

#include <string>

enum LogLevel {
	debug = 0,
	info,
	warning,
	error,
	fatal
};

namespace Logging
{
	class Logger {
	public:
		void logString(LogLevel level, const std::string& msg);
		bool logMessagesPending();
	};

} // namespace Logging

#endif /* SRC_LOGGER_H_ */
