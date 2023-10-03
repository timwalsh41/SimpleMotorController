#include <gtest/gtest.h>
#include <Message.h>
#include <SingletonFactory.h>
#include <BaseLogger.h>
#include <TestLogger.h>
#include <TestTimebase.h>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <ios>
#include <memory>

class MessageTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            SingletonFactory factory;            
            std::shared_ptr<ITimebase> pTimebase = factory.create<ITimebase, TestTimebase>();
            std::shared_ptr<Logging::BaseLogger> pLogger = factory.create<Logging::BaseLogger, Logging::TestLogger>();
        }
        
        // Calculate CRC16 for the input byte array, set the two byte response
        // in the provided crc pointer
        // Uses CRC-CCITT polynomial (0x1021)
        // Reference:
        // https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#Bit_ordering_(endianness)
        // Useful CRC calculator to validate code:
        // http://www.zorc.breitbandkatze.de/crc.html
        uint16_t calculate_crc(uint8_t* bytes, uint16_t len)
        {
            uint16_t rem = 0;

	        auto processWord = [](uint16_t rem, uint8_t byte)
	        {
		        rem ^= static_cast<uint16_t>(byte) << 8;

		        for (size_t i = 0; i < 8; i++)
		        {
			        if (rem & 0x8000)
				        rem = (rem << 1) ^ 0x1021;
			        else
				        rem = rem << 1;
		        }

		        return rem;
	        };

	        for (size_t i = 0; i < len; ++i)
	        {
	            std::cout << "Processing byte " << std::hex << static_cast<int>(bytes[i]) << "\n";
		        rem = processWord(rem, bytes[i]);
	        }

	        return rem;
        } 
};  

// Demonstrate some basic assertions.
TEST_F(MessageTest, LoadRawBytesAndCrc) {

  MessageHandling::Message msg;
  std::vector<uint8_t> msgBytesTooShort{0x22, 0x23};
  
  // Message too short
  EXPECT_FALSE(msg.loadFromBytes(msgBytesTooShort, 0));
  
  // Successful load, bad CRC, with ignored bytes at end
  std::vector<uint8_t> msgBytes{0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29};
  EXPECT_TRUE(msg.loadFromBytes(msgBytes, 4));
  EXPECT_FALSE(msg.checkCRC());
  
  // successful load, correct CRC
  // Note in debug mode, we just use a fixed CRC of 0x6261; this code checks
  // release mode operation with the true CRC calculation
  uint16_t crc = calculate_crc(msgBytes.data(), 2);
  
  msgBytes[2] = static_cast<uint8_t>(crc & 0xFF);
  msgBytes[3] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  EXPECT_TRUE(msg.loadFromBytes(msgBytes, 4));
  EXPECT_TRUE(msg.checkCRC());
  
  // verify set CRC function behaves as expected
  msgBytes[2] = 0x24;
  msgBytes[3] = 0x25;
  EXPECT_TRUE(msg.loadFromBytes(msgBytes, 4));
  msg.setCRC();
  EXPECT_TRUE(msg.checkCRC());
}

TEST_F(MessageTest, MessageParsing) {
    MessageHandling::Message msg;
    std::vector<uint8_t> msgBytes{0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28};
    
    msg.loadFromBytes(msgBytes, 0);
    msg.setCRC();
    
    // verify first byte is loaded as command code
    EXPECT_EQ(msg.getCommandCode(), 0x22);
    
    // now modify command code and verify
    msg.setCommandCode(0x2A);
    EXPECT_EQ(msg.getCommandCode(), 0x2A);
    
    // payload length should be 7 - 3 = 4
    // (7 total bytes, 1 command byte, 2 CRC bytes)
    EXPECT_EQ(msg.getPayloadLength(), 4);
    
    // test setting ack bit in command code
    // (make sure to use the updated command code after we called setCommandCode() above)
    msg.setAck();
    EXPECT_EQ(msg.getCommandCode(), 0x2A | MessageHandling::ACK_BIT_MASK);
    
    msg.clearAck();
    EXPECT_EQ(msg.getCommandCode(), 0x2A);
}

TEST_F(MessageTest, PayloadManipulation) {
    MessageHandling::Message msg;
    std::vector<uint8_t> msgBytes{0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28};
    
    msg.loadFromBytes(msgBytes, 0);
    msg.setCRC();
    
    // verify payload is loaded and retrieved properly
    for (size_t i = 0; i < 4; i++)
        EXPECT_EQ(msgBytes[i+1], msg.getPayload().at(i));
        
    // load new payload as a vector and verify
    std::vector<uint8_t> v{0x1, 0x2, 0x3, 0x4, 0x5};
    msg.setPayload(v);
    EXPECT_EQ(msg.getPayloadLength(), 5);
    for (size_t i = 0; i < v.size(); i++)
        EXPECT_EQ(v.at(i), msg.getPayload().at(i));
        
    // load new payload as a single byte and verify
    uint8_t byte = 0x42;
    msg.setPayload(byte);
    EXPECT_EQ(msg.getPayloadLength(), 1);
    EXPECT_EQ(byte, msg.getPayload().at(0));
}

TEST_F(MessageTest, CopyAndMove) {
    MessageHandling::Message msg;
    std::vector<uint8_t> msgBytes{0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28};
    
    msg.loadFromBytes(msgBytes, 0);
    msg.setCRC();
    
    // copy constructor
    MessageHandling::Message m2{msg};
    EXPECT_EQ(m2.getCommandCode(), msg.getCommandCode());
    EXPECT_EQ(m2.getPayloadLength(), msg.getPayloadLength());
    EXPECT_TRUE(m2.checkCRC());
    
    for (size_t i = 0; i < msg.getPayloadLength(); i++)
        EXPECT_EQ(msg.getPayload().at(i), m2.getPayload().at(i));
        
    // copy assignment
    MessageHandling::Message m3;
    m3 = msg;
    EXPECT_EQ(m3.getCommandCode(), msg.getCommandCode());
    EXPECT_EQ(m3.getPayloadLength(), msg.getPayloadLength());
    EXPECT_TRUE(m3.checkCRC());
    
    for (size_t i = 0; i < msg.getPayloadLength(); i++)
        EXPECT_EQ(msg.getPayload().at(i), m3.getPayload().at(i));
        
    // move constructor - compare against one of the derivative messages since
    // we've hollowed out the original
    MessageHandling::Message m4{std::move(msg)};
    EXPECT_EQ(m4.getCommandCode(), msg.getCommandCode());
    EXPECT_EQ(m4.getPayloadLength(), m3.getPayloadLength());
    EXPECT_TRUE(m4.checkCRC());
    EXPECT_EQ(msg.getPayloadLength(), 0);
    
    for (size_t i = 0; i < m3.getPayloadLength(); i++)
        EXPECT_EQ(m3.getPayload().at(i), m4.getPayload().at(i));
    
    // move assignment operator - compare against one of the derivative messages since
    // we've hollowed out the original
    MessageHandling::Message m5;
    m5 = std::move(m2);
    EXPECT_EQ(m5.getCommandCode(), msg.getCommandCode());
    EXPECT_EQ(m5.getPayloadLength(), m3.getPayloadLength());
    EXPECT_TRUE(m5.checkCRC());
    EXPECT_EQ(m2.getPayloadLength(), 0);
    
    for (size_t i = 0; i < m3.getPayloadLength(); i++)
        EXPECT_EQ(m3.getPayload().at(i), m5.getPayload().at(i));
}

TEST_F(MessageTest, ToByteArray) {
    MessageHandling::Message msg;
    std::vector<uint8_t> msgBytes{0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28};
    uint16_t crc = calculate_crc(msgBytes.data(), 5);
    msgBytes[5] = static_cast<uint8_t>(crc & 0xFF);
    msgBytes[6] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    
    msg.loadFromBytes(msgBytes, 0);
    
    // the toByteArray function checks the CRC, so it needs to be set properly
    msg.setCRC();
    
    uint8_t* bytes{nullptr};
    uint32_t len{0};
    EXPECT_TRUE(msg.toByteArray(&bytes, len));
    
    // length should be equal to payload length plus 3 (one byte command code plus two byte CRC)
    EXPECT_EQ(len, msg.getPayloadLength() + 3);
    
    for (size_t i = 0; i < 7; i++)
    {
        std::cout << i << ". " << std::hex << msgBytes[i] << " <==> " << bytes[i] << "\n";
        EXPECT_EQ(msgBytes[i], bytes[i]);
   }  
        
    if (len > 0)
        delete[] bytes;
}

