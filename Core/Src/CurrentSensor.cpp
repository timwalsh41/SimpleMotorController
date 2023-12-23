/*
 * CurrentSensor.cpp
 *
 *  Created on: Jun 12, 2023
 *      Author: tim
 */

#include "CurrentSensor.h"
#include "SingletonFactory.h"
#include <cstring>
#include <algorithm>
#include <numeric>

namespace Sensors
{

	CurrentSensor::CurrentSensor()
	{
		SingletonFactory factory;
		m_pLogger = factory.retrieve<Logging::BaseLogger>();

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

				*m_storageBufferPtr = sum/CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT*ADC_TO_MA - m_zeroCurrentOffset;

				if (++m_storageBufferPtr == m_storageBuffer.end())
					m_storageBufferPtr = m_storageBuffer.begin();
			}
			m_bBufferReady = false;
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
		{
			m_pLogger->logString(error, "Invalid payload in getMotorCurrent command");
			return false;
		}

		// retrieve the desired number of milliseconds of data from the payload
		uint16_t millisecondsOfData{0};
		std::memcpy(&millisecondsOfData, msg.getPayload().data(), 2);

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
			m_pLogger->logString(error, std::string("Unable to allocate memory for returning motor current array"));
			return false;
		}

		// load in number of samples of float data to first two bytes of payload
		uint32_t numSamples = millisecondsOfData*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
		payload.push_back(static_cast<uint8_t>((numSamples >> 0) & 0xFF));
		payload.push_back(static_cast<uint8_t>((numSamples >> 8) & 0xFF));

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

	// calculate and return the mean current over requested time range
	bool CurrentSensor::getMeanCurrent(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		// motor current request has a two byte payload equal to the number of milliseconds of data to send
		if (msg.getPayload().size() != 2)
		{
			m_pLogger->logString(error, "Invalid payload in getMeanCurrent command");
			return false;
		}

		// retrieve the desired number of milliseconds of data from the payload
		uint16_t millisecondsOfData{0};
		std::memcpy(&millisecondsOfData, msg.getPayload().data(), 2);

		// TODO: this relies on system tick time being one millisecond
		int numDataPoints = static_cast<int32_t>(millisecondsOfData)*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;

		if (numDataPoints > CURRENT_SENSOR_STORAGE_LENGTH)
		{
			// note we're doing integer divide here to get the maximum number of whole number milliseconds
			// of data we can transmit
			millisecondsOfData = CURRENT_SENSOR_STORAGE_LENGTH / CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
			numDataPoints = static_cast<uint32_t>(millisecondsOfData)*CURRENT_SENSOR_MEASUREMENTS_PER_TICK;
		}

		// the required size of the return data is just the size of a float
		payload.reserve(sizeof(float));

		if (payload.capacity() != sizeof(float))
		{
			m_pLogger->logString(error, std::string("Unable to allocate memory for returning motor mean current"));
			return false;
		}

		float meanCurrent = 0;

		// To actually copy data into the payload, recall that we are always sending the most recent numDataPoints
		// points. The most recent point is always at m_storageBufferPtr - 1 (with wrapping for circular buffer). So we
		// want to get the numDataPoints points prior to m_storageBufferPtr, with wrapping.
		if ((m_storageBufferPtr - m_storageBuffer.begin()) >= numDataPoints)
		{
			for (auto it = m_storageBufferPtr - numDataPoints; it != m_storageBufferPtr; ++it)
				meanCurrent += *it;
		}
		else
		{
			// to wrap buffer, we'll first find the number of data points we need from the end of the buffer
			int valuesAtEndOfBuffer = numDataPoints - (m_storageBufferPtr - m_storageBuffer.begin());

			// grab points at the end and insert into payload
			for (auto it = m_storageBuffer.end() - valuesAtEndOfBuffer; it != m_storageBuffer.end(); ++it)
				meanCurrent += *it;

			// now grab the remaining points up to the most recent at the location prior to m_storageBufferPtr
			for (auto it = m_storageBuffer.begin(); it != m_storageBufferPtr; ++it)
				meanCurrent += *it;
		}

		meanCurrent /= numDataPoints;

		uint32_t* p_meanCurrent;
		p_meanCurrent = reinterpret_cast<uint32_t*>(&meanCurrent);

		payload.push_back(static_cast<uint8_t>((*p_meanCurrent >> 0) & 0xFF));
		payload.push_back(static_cast<uint8_t>((*p_meanCurrent >> 8) & 0xFF));
		payload.push_back(static_cast<uint8_t>((*p_meanCurrent >> 16) & 0xFF));
		payload.push_back(static_cast<uint8_t>((*p_meanCurrent >> 24) & 0xFF));

		return true;
	}

	bool CurrentSensor::zeroCurrentSensor(const MessageHandling::Message& msg, std::vector<uint8_t>& payload)
	{
		if (msg.getPayload().size() > 0)
		{
			m_pLogger->logString(error, "Invalid payload in zeroCurrentSensor command");
			return false;
		}

		zeroCurrentSensor();

		return true;
	}

	void CurrentSensor::zeroCurrentSensor()
	{
		// to zero the current sensor, we'll take the average of the circular buffer
		m_zeroCurrentOffset += std::accumulate(m_storageBuffer.begin(), m_storageBuffer.end(), 0.) / m_storageBuffer.size();
	}

	// Clear the storage buffer and set the storage buffer pointer to the start of the array
	void CurrentSensor::clearStorageBuffer()
	{
		m_storageBuffer.assign(CURRENT_SENSOR_STORAGE_LENGTH, m_zeroCurrentOffset);
		m_storageBufferPtr = m_storageBuffer.begin();
	}

} /* namespace CurrentSensor */
