#include <gtest/gtest.h>
#include <Message.h>
#include <Logger.h>
#include <stdint.h>

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
  
  MessageHandling::Message msg;
  uint8_t msgBytes[] = {0x22, 0x23, 0x24, 0x25};
  
  EXPECT_FALSE(msg.loadFromBytes(4, msgBytes));
}
