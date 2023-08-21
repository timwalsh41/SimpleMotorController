/*
 * CurrentSensor.cpp
 *
 *  Created on: Jun 12, 2023
 *      Author: tim
 */

#include "CurrentSensor.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>
#include <iostream>

extern Logging::Logger theLogger;

namespace Sensors
{

	CurrentSensor::CurrentSensor() {

		for (size_t i = 0; i < CURRENT_SENSOR_BUFFER_SIZE; ++i)
		{
			m_pBufferA[i] = 0;
			m_pBufferB[i] = 0;
		}

		m_storageBuffer.reserve(CURRENT_SENSOR_STORAGE_LENGTH);
		clearStorageBuffer();

		m_pCurrentBuffer = m_pBufferA;
	}

	// Process raw current measurements into averaged values and enter into storage buffer
	void CurrentSensor::processCurrentMeasurements()
	{
		if (m_bBufferReady)
		{
			uint32_t* pBuffer = (m_pCurrentBuffer == m_pBufferA) ? m_pBufferB : m_pBufferA;
			float sum = 0;

			for (uint32_t i = 0; i < CURRENT_SENSOR_BUFFER_SIZE; i += CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT)
			{
				sum = 0;
				for (uint32_t j = 0; j < CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT; j++)
				{
					sum += pBuffer[i+j];
				}

				*m_storageBufferPtr = (sum / CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT - CURRENT_SENSOR_ZERO_OFFSET) *
														 ADC_TO_MA;

				if (++m_storageBufferPtr == m_storageBuffer.end())
					m_storageBufferPtr = m_storageBuffer.begin();
			}
		}
	}

	// Ping pong the buffers when one fills up with ADC readings
	void CurrentSensor::swapBuffer()
	{
		if (m_pCurrentBuffer == m_pBufferA)
			m_pCurrentBuffer = m_pBufferB;
		else
			m_pCurrentBuffer = m_pBufferA;

		m_bBufferReady = true;
	}

	// Get pointer to current measurement buffer
	uint32_t* CurrentSensor::getCurrentBuffer() const
	{
		return m_pCurrentBuffer;
	}

	// Load values in chronological order from storage buffer into the provided output buffer
	// Requires that outputBuffer has at least numSamples allocated
	bool CurrentSensor::getMotorCurrent(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// motor current request has a two byte payload equal to the number of milliseconds of data to send
		if (msg.getPayload().size() != 2)
			return false;

		// retrieve the desired number of milliseconds of data from the payload
		uint16_t millisecondsOfData{0};
		std::memcpy(&millisecondsOfData, msg.getPayload().data(), 2);

		std::cout << "Number of milliseconds of data (desired) = " << millisecondsOfData << "\n";

		// TODO: this relies on system tick time being one millisecond
		int numDataPoints = static_cast<int32_t>(millisecondsOfData)*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;

		if (numDataPoints > CURRENT_SENSOR_STORAGE_LENGTH)
		{
			// note we're doing integer divide here to get the maximum number of whole number milliseconds
			// of data we can transmit
			millisecondsOfData = CURRENT_SENSOR_STORAGE_LENGTH / CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
			numDataPoints = static_cast<uint32_t>(millisecondsOfData)*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
		}

		// the required size of the return data is two bytes for the number of data points plus 4 bytes per data point
		uint32_t payloadSize = 2 + sizeof(float)*numDataPoints;
		payload.reserve(payloadSize);

		if (payload.capacity() != payloadSize)
		{
			theLogger.logString(error, std::string("Unable to allocate memory for returning motor current array"));
			return false;
		}

		// load in number of milliseconds of data to first two bytes of payload
		payload.push_back(static_cast<uint8_t>((millisecondsOfData >> 8) & 0xFF));
		payload.push_back(static_cast<uint8_t>(millisecondsOfData & 0xFF));

		std::cout << "Number of milliseconds of data (actual) = " << millisecondsOfData << "\n";
		std::cout << "Number of measurement data points = " << numDataPoints << "\n";
		std::cout << "Number of bytes = " << numDataPoints*sizeof(float) << "\n";
		std::cout << "Storage pointer offset: " << m_storageBufferPtr - m_storageBuffer.begin() << "\n";

		uint8_t transferBuffer[sizeof(float)];

		// To actually copy data into the payload, recall that we are always sending the most recent numDataPoints
		// points. The most recent point is always at m_storageBufferPtr - 1 (with wrapping for circular buffer). So we
		// want to get the numDataPoints points prior to m_storageBufferPtr, with wrapping.
		if ((m_storageBufferPtr - m_storageBuffer.begin()) >= numDataPoints)
		{
			for (auto it = m_storageBufferPtr - numDataPoints; it != m_storageBufferPtr; ++it)
			{
				std::memcpy(transferBuffer, &(*it), sizeof(float));
				payload.insert(payload.end(), transferBuffer, transferBuffer+sizeof(float));
			}
		}
		else
		{
			// to wrap buffer, we'll first find the number of data points we need from the end of the buffer
			int valuesAtEndOfBuffer = numDataPoints - (m_storageBufferPtr - m_storageBuffer.begin());

			std::cout << "Wrapping buffer, grabbing " << valuesAtEndOfBuffer << " points from the end\n";

			// grab points at the end and insert into payload
			for (auto it = m_storageBuffer.end() - valuesAtEndOfBuffer; it != m_storageBuffer.end(); ++it)
			{
				std::memcpy(transferBuffer, &(*it), sizeof(float));
				payload.insert(payload.end(), transferBuffer, transferBuffer+sizeof(float));
			}

			// now grab the remaining points up to the most recent at the location prior to m_storageBufferPtr
			for (auto it = m_storageBuffer.begin(); it != m_storageBufferPtr; ++it)
			{
				std::memcpy(transferBuffer, &(*it), sizeof(float));
				payload.insert(payload.end(), transferBuffer, transferBuffer+sizeof(float));
			}
		}

		return true;
	}

	void CurrentSensor::zeroCurrentSensor()
	{
		// TODO: figure out how to zero current sensor
		// could use an average of recent points
	}

	// Clear the storage buffer and set the storage buffer pointer to the start of the array
	void CurrentSensor::clearStorageBuffer()
	{
		m_storageBuffer.assign(CURRENT_SENSOR_STORAGE_LENGTH, 0);
		m_storageBufferPtr = m_storageBuffer.begin();
	}

} /* namespace CurrentSensor */
