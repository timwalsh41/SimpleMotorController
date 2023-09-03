#include <gtest/gtest.h>
#include <Message.h>
#include <CurrentSensor.h>
#include <stdint.h>
#include <iostream>
#include <ios>
#include <algorithm>
#include <cstring>
#include <vector>

// calculate the number of times we need to fill the measurement buffer to get a full storage buffer 
constexpr int numberOfMeasurementBuffersPerStorageBuffer = Sensors::CURRENT_SENSOR_STORAGE_LENGTH/(Sensors::CURRENT_SENSOR_BUFFER_SIZE/Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT) + 1;

class CurrentSensorTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {

        }
        
        void LoadData(int startValue, int numSamples, int sampleOffset)
        {
            int sampleCount{0};
            int sampleValue{startValue};
            
            int samplesToRun = std::clamp(numSamples, static_cast<int>(0), static_cast<int>(Sensors::CURRENT_SENSOR_BUFFER_SIZE));
            
            for (size_t i = 0; i < samplesToRun; ++i)
            {
                m_sensor.getCurrentBuffer()[i] = sampleValue;
                if (++sampleCount == Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT)
                {
                    m_sensor.getCurrentBuffer()[i] += sampleOffset;
                    sampleCount = 0;
                    ++sampleValue;
                }
            }
                
            m_sensor.swapBuffer();
        }
        
        Sensors::CurrentSensor m_sensor{};
        MessageHandling::Message msg{};
};

// Test case - load up the measurement buffer with a single batch and then retrieve it without having to wrap the storage buffer to
// retrieve data
TEST_F(CurrentSensorTest, StandardInsertSingleBuffer) {
  unsigned int dataLength_ms = 1;
  int numberOfDataPoints = dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK*sizeof(float);
  
  m_sensor.clearStorageBuffer();
  
  std::vector<uint8_t> msgBytes{0x22, static_cast<uint8_t>(dataLength_ms & 0xFF), static_cast<uint8_t>((dataLength_ms >> 8) & 0xFF), 0x25, 0x26};
  EXPECT_TRUE(msg.loadFromBytes(msgBytes, 0));
  EXPECT_FALSE(msg.checkCRC());
  
  // For this to work, we need to request data (dataLength_ms) that is less than a full measurement buffer, equal to 
  // CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 measurements
  ASSERT_LE((dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK), (Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10));
  
  // Load data into the current sensor measurement buffer; this will load one tick plus 10 measurements worth of data
  // The exact amount of data loaded is relevant since we need to know what the most recent data received is for checking
  // it below
  LoadData(0, Sensors::CURRENT_SENSOR_BUFFER_SIZE, 0);
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


// Test case - load up the measurement buffer with multiple batches and retrieve it with a wrap of the storage buffer
TEST_F(CurrentSensorTest, StandardInsertWrapBuffer) {
    // we are filling the storage buffer to wrap by less than one tick, so requesting at least 2 ms of data should cause a wrap
    uint16_t dataLength_ms = 3;
    int maxStorageBufferValue = numberOfMeasurementBuffersPerStorageBuffer*Sensors::CURRENT_SENSOR_BUFFER_SIZE/Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT - 1;
    int initialSampleValue = maxStorageBufferValue - dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 1;
    std::cout << "Initial buffer value = " << initialSampleValue << "\n";
 
    m_sensor.clearStorageBuffer();

    std::cout << "Measurement buffer size (in filtered storage buffer data points) = " << Sensors::CURRENT_SENSOR_BUFFER_SIZE/Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT << "\n";
    std::cout << "Storage buffer size (in data points) = " << Sensors::CURRENT_SENSOR_STORAGE_LENGTH << "\n";
    std::cout << "Filling measurement buffer " << numberOfMeasurementBuffersPerStorageBuffer << " times to fill storage buffer\n";
    
    
    int startValue = 0;
    for (size_t i = 0; i < numberOfMeasurementBuffersPerStorageBuffer; ++i)
    {
        LoadData(startValue, Sensors::CURRENT_SENSOR_BUFFER_SIZE, 0);
        m_sensor.processCurrentMeasurements();
        startValue += Sensors::CURRENT_SENSOR_BUFFER_SIZE/Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT;
    }
    
    std::vector<uint8_t> msgBytes{0x22, static_cast<uint8_t>(dataLength_ms & 0xFF), static_cast<uint8_t>((dataLength_ms >> 8) & 0xFF), 0x25, 0x26};
    EXPECT_TRUE(msg.loadFromBytes(msgBytes, 0));
    EXPECT_FALSE(msg.checkCRC());
    
    std::vector<uint8_t> measurements{};
    EXPECT_TRUE(m_sensor.getMotorCurrent(msg, measurements));
    
    float f;
    int count = 0;
    int currentDataPoint = initialSampleValue;
    for (auto it = measurements.begin()+2; it < measurements.end(); it=it+4)
    {
        std::memcpy(&f, &(*it), sizeof(float));
        EXPECT_FLOAT_EQ(currentDataPoint, f);
        std::cout << count << ". " << f << "\n";
        currentDataPoint += 1;
    }
    
    // force failure to get printout
    // EXPECT_TRUE(false);
}

// Test case - load up measurement buffer to get floating point return values
TEST_F(CurrentSensorTest, FloatInsertSingleBuffer) {
  unsigned int dataLength_ms = 1;
  int numberOfDataPoints = dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK*sizeof(float);
  
  m_sensor.clearStorageBuffer();
  
  std::vector<uint8_t> msgBytes{0x22, static_cast<uint8_t>(dataLength_ms & 0xFF), static_cast<uint8_t>((dataLength_ms >> 8) & 0xFF), 0x25, 0x26};
  EXPECT_TRUE(msg.loadFromBytes(msgBytes, 0));
  EXPECT_FALSE(msg.checkCRC());
  
  // For this to work, we need to request data (dataLength_ms) that is less than a full measurement buffer, equal to 
  // CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 measurements
  ASSERT_LE((dataLength_ms*Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK), (Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10));
  
  // Load data into the current sensor measurement buffer; this will load one tick plus 10 measurements worth of data
  // The exact amount of data loaded is relevant since we need to know what the most recent data received is for checking
  // it below
  // Here we are using a sample offset of 1 so we should get floating point return values, what this does is to add in an offset of 1 to
  // each batch of CRURENT_SAMPLE_SAMPLES_PER_MEASUREMENT values that are averaged together from the measurement buffer to get a single
  // storage buffer value. This just bumps the value off from the nominal integral value that would otherwise be applied by LoadData. We 
  // account for that shift when we check values below.
  LoadData(0, Sensors::CURRENT_SENSOR_BUFFER_SIZE, 1);
  m_sensor.processCurrentMeasurements();
  
  std::vector<uint8_t> measurements{};
  EXPECT_TRUE(m_sensor.getMotorCurrent(msg, measurements));
  EXPECT_EQ(measurements.size(), numberOfDataPoints + 2);
  
  // Now we'll check the values in the buffer. For each measurement, we need to convert from 4 bytes to float, which we'll accomplish with
  // std::memcpy. The most recent data will be last in the measurement array. The most recent data will be the last data point we put in the
  // measurement buffer with LoadData, which is equal to CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 - 1 (since we start at zero)
  float lastDataPoint = Sensors::CURRENT_SENSOR_MEASUREMENTS_PER_TICK + 10 - 1;
  float firstDataPoint = lastDataPoint - numberOfDataPoints/sizeof(float) + 1;
  
  // here we will add in the sample offset expected from the load data function
  float currentDataPoint = firstDataPoint + 1/static_cast<float>(Sensors::CURRENT_SENSOR_SAMPLES_PER_MEASUREMENT);
  
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
