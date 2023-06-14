/*
 * CurrentSensor.h
 *
 *  Created on: Jun 12, 2023
 *      Author: tim
 */

#ifndef SRC_CURRENTSENSOR_H_
#define SRC_CURRENTSENSOR_H_

#include "stdint.h"

// number of high-speed ADC samples to take at each measurement time point
constexpr uint32_t CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT = 5;

// number of measurement time points per system tick (e.g. per main loop call)
constexpr uint32_t CURRENT_SENSOR_MEASUREMENTS_PER_TICK = 50;

// number of ticks worth of processed measurements to hold in circular storage buffer
constexpr uint32_t CURRENT_SENSOR_STORAGE_LENGTH_TICKS = 50;

// number of raw samples recorded in measurement buffers
// we oversize the buffer to try to ensure we don't fill two buffers in one
// tick interval
constexpr uint32_t CURRENT_SENSOR_BUFFER_SIZE = (CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10) * \
				   CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT;

// number of processed samples recorded in storage buffer
constexpr uint32_t CURRENT_SENSOR_STORAGE_LENGTH = CURRENT_SENSOR_MEASUREMENTS_PER_TICK * \
		           CURRENT_SENSOR_STORAGE_LENGTH_TICKS;

// conversion factor from ADC number to mA
// TODO: Set conversion factor
constexpr float ADC_TO_MA = 1;

class CurrentSensor {
public:
	CurrentSensor();
	virtual ~CurrentSensor();

	void processCurrentMeasurements();
	void swapBuffer();

	uint32_t* getCurrentBuffer() const;

	bool getStorageBuffer(uint16_t* outputBuffer, uint32_t& numSamples);

private:
	// Ping pong buffers for storing ADC readings and processing them in main loop
	uint32_t m_pBufferA[CURRENT_SENSOR_BUFFER_SIZE];
	uint32_t m_pBufferB[CURRENT_SENSOR_BUFFER_SIZE];

	// m_pCurrentBuffer pointer always points to the buffer currently in use for
	// measurement
	uint32_t* m_pCurrentBuffer{nullptr};

	// storage buffer and a pointer to use for circular storage
	float m_pStorageBuffer[CURRENT_SENSOR_STORAGE_LENGTH];
	uint32_t m_storageBufferPtr{0};

	// flag for when a buffer is full and ready to be processed
	bool m_bBufferReady{false};
};

#endif /* SRC_CURRENTSENSOR_H_ */
