/*
 * CurrentSensor.cpp
 *
 *  Created on: Jun 12, 2023
 *      Author: tim
 */

#include "CurrentSensor.h"
#include "Logger.h"
#include <algorithm>
#include <cstring>
#include <algorithm>

extern Logging::Logger theLogger;

namespace Sensors
{

	CurrentSensor::CurrentSensor() {

		for (uint32_t i = 0; i < CURRENT_SENSOR_BUFFER_SIZE; ++i)
		{
			m_pBufferA[i] = 0;
			m_pBufferB[i] = 0;
		}

		for (uint32_t i = 0; i < CURRENT_SENSOR_STORAGE_LENGTH; ++i)
			m_pStorageBuffer[i] = 0;

		m_pCurrentBuffer = m_pBufferA;
	}

	// Process raw current measurements into averaged values and enter into storage buffer
	void CurrentSensor::processCurrentMeasurements()
	{
		if (m_bBufferReady)
		{
			uint32_t* pBuffer = (m_pCurrentBuffer == m_pBufferA) ? m_pBufferA : m_pBufferB;
			float sum = 0;
			for (uint32_t i = 0; i < CURRENT_SENSOR_BUFFER_SIZE; i += CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT)
			{
				sum = 0;
				for (uint32_t j = 0; j < CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT; j++)
					sum += pBuffer[i+j];

				m_pStorageBuffer[m_storageBufferPtr++] = (sum / CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT - CURRENT_SENSOR_ZERO_OFFSET) *
														 ADC_TO_MA;
				if (m_storageBufferPtr == CURRENT_SENSOR_STORAGE_LENGTH)
					m_storageBufferPtr = 0;
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
	// TODO: this function is not actually used
	bool CurrentSensor::getStorageBuffer(uint16_t* outputBuffer, uint32_t& numSamples)
	{
		// we cap the number of samples at the length of the storage buffer
		numSamples = std::min(numSamples, CURRENT_SENSOR_STORAGE_LENGTH);

		// a pointer to track the index we're copying into for the output buffer, we'll start from the
		// end with the most recent data
		int32_t outputBufferPtr = numSamples - 1;

		// start populating the output buffer from the newest samples in the storage buffer
		for (int32_t i = m_storageBufferPtr-1; i >= 0; --i)
		{
			outputBuffer[outputBufferPtr--] = static_cast<uint16_t>(m_pStorageBuffer[i]);
			if (outputBufferPtr < 0)
				// we've finished filling the output buffer
				return true;
		}

		// wrap the circular buffer
		for (int32_t i = CURRENT_SENSOR_STORAGE_LENGTH-1; i >= static_cast<int32_t>(m_storageBufferPtr); --i)
		{
			outputBuffer[outputBufferPtr--] = static_cast<uint16_t>(m_pStorageBuffer[i]);
			if (outputBufferPtr < 0)
				// we've finished filling the output buffer
				return true;
		}

		// we shouldn't ever return from here, so this indicates something has gone wrong
		return false;
	}

	bool CurrentSensor::getMotorCurrent(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// motor current request has a two byte payload equal to the number of milliseconds of data to send
		if (msg.getPayload().size() != 2)
			return false;

		// retrieve the desired number of milliseconds of data from the payload
		uint16_t millisecondsOfData{0};
		std::memcpy(&millisecondsOfData, msg.getPayload().data(), 2);

		// TODO: this relies on system tick time being one millisecond
		uint32_t numDataPoints = static_cast<uint32_t>(millisecondsOfData)*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
		numDataPoints = std::min(numDataPoints, CURRENT_SENSOR_STORAGE_LENGTH);

		// the required size of the return data is two bytes for the number of data points plus 4 bytes per data point
		uint32_t payloadSize = 2 + sizeof(float)*numDataPoints;
		payload.reserve(payloadSize);

		if (payload.capacity() != payloadSize)
		{
			theLogger.logString(error, std::string("Unable to allocate memory for returning motor current array"));
			return false;
		}

		uint8_t transferBuffer[sizeof(float)];
		for (int32_t i = m_storageBufferPtr-1; i >= 0; --i)
		{
			std::memcpy(transferBuffer, &m_pStorageBuffer[i], sizeof(float));
			payload.insert(payload.end(), &transferBuffer[0], &transferBuffer[sizeof(float)]);
		}

		for (int32_t i = CURRENT_SENSOR_STORAGE_LENGTH-1; i >= static_cast<int32_t>(m_storageBufferPtr); --i)
		{
			std::memcpy(transferBuffer, &m_pStorageBuffer[i], sizeof(float));
			payload.insert(payload.end(), &transferBuffer[0], &transferBuffer[sizeof(float)]);
		}

		return true;
	}

	void zeroCurrentSensor()
	{
		// TODO: figure out how to zero current sensor
		// could use an average of recent points
		assert(false);
	}

} /* namespace CurrentSensor */
