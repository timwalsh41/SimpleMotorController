#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Message.h>
#include <TestTimebase.h>
#include <MockMessageHandler.h>
#include <SerialParser.h>
#include <iostream>

TEST(SerialParserTest, DetectStartAndStopSequence) {

    TestTimebase timebase;
    MockMessageHandler messageHandler;
    MessageHandling::SerialParser sp{&timebase, &messageHandler};

}

// TODO:
// - Set up mock message handler; we judge success by whether the message handler is called to load the message
// - Rewrite serial parser to use dependency injection for message handler
// - Replace raw arrays with vectors for incoming buffer, candidate sequence, and candidate message

// Tests:
// detect start sequence from random characters
// detect stop sequence in message
// timeout during message transmission
// message exceeding maximum length

