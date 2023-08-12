#include <gtest/gtest.h>
#include <Message.h>
#include <Logger.h>
#include <stdint.h>
#include <iostream>
#include <ios>

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


// Demonstrate some basic assertions.
TEST(MessageTest, LoadRawBytes) {
  
  MessageHandling::Message msg;
  uint8_t msgBytesTooShort[] = {0x22, 0x23};
  
  // Message too short
  EXPECT_FALSE(msg.loadFromBytes(2, msgBytesTooShort));
  
  // Successful load, bad CRC
  uint8_t msgBytes[] = {0x22, 0x23, 0x24, 0x25};
  EXPECT_TRUE(msg.loadFromBytes(4, msgBytes));
  EXPECT_FALSE(msg.checkCRC());
  
  // successful load, correct CRC
  // Note in debug mode, we just use a fixed CRC of 0x6261; this code checks
  // release mode operation with the true CRC calculation
  uint16_t crc = calculate_crc(msgBytes, 2);
  
  std::cout << "Test calculation: " << std::hex << crc << "\n";
  
  msgBytes[2] = static_cast<uint8_t>(crc & 0xFF);
  msgBytes[3] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  EXPECT_TRUE(msg.loadFromBytes(4, msgBytes));
  EXPECT_TRUE(msg.checkCRC());
  
  msg.setCRC();
  std::cout << "Member func crc: " << std::hex << msg.getCRC() << "\n";
}
