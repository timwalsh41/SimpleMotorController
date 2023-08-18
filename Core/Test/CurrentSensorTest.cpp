#include <gtest/gtest.h>
#include <Message.h>
#include <CurrentSensor.h>
#include <stdint.h>
#include <iostream>
#include <ios>
#include <algorithm>
#include <cstring>
#include <vector>

class CurrentSensorTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            
        }
        
        void LoadData(int startValue, int numSamples)
        {
            int sampleCount{0};
            int sampleValue{startValue};
            
            int samplesToRun = std::clamp(numSamples, static_cast<int>(0), static_cast<int>(Sensors::CURRENT_SENSOR_BUFFER_SIZE));
            
            for (size_t i = 0; i < samplesToRun; ++i)
            {
                m_sensor.getCurrentBuffer()[i] = sampleValue;
                if (++sampleCount == Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT)
                {
                    sampleCount = 0;
                    ++sampleValue;
                }
            }
                
            m_sensor.swapBuffer();
        }
        
        Sensors::CurrentSensor m_sensor;  
};

TEST_F(CurrentSensorTest, StandardInsertSingleBuffer) {
  unsigned int dataLength_ms = 2;
  int numberOfDataPoints = dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK*sizeof(float);
  
  MessageHandling::Message msg;
  uint8_t msgBytes[] = {0x22, static_cast<uint8_t>(dataLength_ms & 0xFF), static_cast<uint8_t>((dataLength_ms >> 8) & 0xFF), 0x25, 0x26};
  EXPECT_TRUE(msg.loadFromBytes(5, msgBytes));
  EXPECT_FALSE(msg.checkCRC());
  
  // For this to work, we need to request data (dataLength_ms) that is less than a full measurement buffer, equal to 
  // CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 measurements
  ASSERT_LE((dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK), (Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10));
  
  // Load data into the current sensor measurement buffer; this will load one tick plus 10 measurements worth of data
  // The exact amount of data loaded is relevant since we need to know what the most recent data received is for checking
  // it below
  LoadData(0, Sensors::CURRENT_SENSOR_BUFFER_SIZE);
  m_sensor.processCurrentMeasurements();
  
  std::vector<uint8_t> measurements{};
  EXPECT_TRUE(m_sensor.getMotorCurrent(msg, measurements));
  EXPECT_EQ(measurements.size(), numberOfDataPoints + 2);
  
  // Now we'll check the values in the buffer. For each measurement, we need to convert from 4 bytes to float, which we'll accomplish with
  // std::memcpy. The most recent data will be last in the measurement array. The most recent data will be the last data point we put in the
  // measurement buffer with LoadData, which is equal to CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 - 1 (since we start at zero)
  float lastDataPoint = Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 - 1;
  float firstDataPoint = lastDataPoint - numberOfDataPoints/sizeof(float) + 1;
  float currentDataPoint = firstDataPoint;
  
  float f;
  int count = 0;
  for (auto it = measurements.begin()+2; it < measurements.end(); it=it+4)
  {
    std::memcpy(&f, &(*it), sizeof(float));
    EXPECT_FLOAT_EQ(currentDataPoint, f);
    currentDataPoint += 1;
  }
  
  // force failure to get printout
  // EXPECT_TRUE(false);
}

// TODO: TEST CASES
// - Standard insert, multiple buffer
// - Wrap insert, single/multiple buffer?
// - Consider potential off by one errors
