/*
 * OutputGateway.h
 *
 *  Created on: May 26, 2023
 *      Author: tim
 */

#ifndef INC_OUTPUTGATEWAY_H_
#define INC_OUTPUTGATEWAY_H_

#include <string>
#include <string_view>

enum LogLevel {
	debug = 0,
	info,
	warning,
	error,
	fatal
};

class OutputGateway
{
public:
	void logString(LogLevel level, const std::string& msg);
	void setReceiving() { m_receiving = true; }
	void setTransmitting() { m_receiving = false; }

private:
	void sendStringOutput(const std::string& outString);
	bool m_receiving = true;
	static constexpr std::string_view logLevelString[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

};



#endif /* INC_OUTPUTGATEWAY_H_ */
