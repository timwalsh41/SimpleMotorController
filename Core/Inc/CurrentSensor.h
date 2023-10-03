/*
 * CurrentSensor.h
 *
 *  Created on: Jun 12, 2023
 *      Author: tim
 */

#ifndef SRC_CURRENTSENSOR_H_
#define SRC_CURRENTSENSOR_H_

#include "Message.h"
#include "BaseLogger.h"
#include "stdint.h"
#include <memory>

namespace Sensors
{

	// number of high-speed ADC samples to take at each measurement time point
	constexpr int CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT = 5;

	// number of measurement time points per system tick (e.g. per main loop call)
	constexpr int CURRENT_SENSOR_MEASUREMENTS_PER_TICK = 20;

	// number of ticks worth of processed measurements to hold in circular storage buffer
	constexpr int CURRENT_SENSOR_STORAGE_LENGTH_TICKS = 50;

	// number of raw samples recorded in measurement buffers
	// we size the buffer to try to ensure we don't fill two buffers in one
	// tick interval so that we can call processCurrentMeasurements() in between
	constexpr int CURRENT_SENSOR_BUFFER_SIZE = (CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10) * \
					   CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT;

	// number of processed samples recorded in storage buffer
	constexpr int CURRENT_SENSOR_STORAGE_LENGTH = CURRENT_SENSOR_MEASUREMENTS_PER_TICK * \
					   CURRENT_SENSOR_STORAGE_LENGTH_TICKS;

	// default zero offset for current sensor reading
	constexpr uint32_t CURRENT_SENSOR_ZERO_OFFSET = 0;

	// conversion factor from ADC number to mA
	// TODO: Set conversion factor
	constexpr float ADC_TO_MA = 1;

	class CurrentSensor {
	public:
		CurrentSensor();

		void processCurrentMeasurements();
		void swapBuffer();
		void clearStorageBuffer();

		uint32_t* getCurrentBuffer() const;

		// command handler to get motor current data
		bool getMotorCurrent(const MessageHandling::Message& msg, std::vector<uint8_t>& payload);

		void zeroCurrentSensor();

	private:
		// Ping pong buffers for storing ADC readings and processing them in main loop
		uint32_t m_pBufferA[CURRENT_SENSOR_BUFFER_SIZE];
		uint32_t m_pBufferB[CURRENT_SENSOR_BUFFER_SIZE];

		// m_pCurrentBuffer pointer always points to the buffer currently in use for
		// measurement
		uint32_t* m_pCurrentBuffer{nullptr};

		// storage buffer and a pointer to use for circular storage;
		std::vector<float> m_storageBuffer;
		std::vector<float>::iterator m_storageBufferPtr;

		// flag for when a buffer is full and ready to be processed
		bool m_bBufferReady{false};

		// zero offset, in ADC counts
		uint32_t m_zeroCurrentOffset{CURRENT_SENSOR_ZERO_OFFSET};

		std::shared_ptr<Logging::BaseLogger> m_pLogger;
	};

} /* namespace CurrentSensor */
#endif /* SRC_CURRENTSENSOR_H_ */
