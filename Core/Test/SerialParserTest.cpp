#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <Message.h>
#include <TestTimebase.h>
#include <SingletonFactory.h>
#include <MockMessageHandler.h>
#include <SerialParser.h>
#include <CommandInterface.h>
#include <ILogger.h>
#include <MockLogger.h>
#include <iostream>
#include <deque>
#include <chrono>
#include <thread>

class SerialParserTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            SingletonFactory factory;
            std::shared_ptr<ITimebase> pTimebase = factory.create<ITimebase, TestTimebase>();
            std::shared_ptr<Logging::ILogger> pLogger = factory.create<Logging::ILogger, Logging::MockLogger>();
        }
        
        MockMessageHandler m_messageHandler{};
};

TEST_F(SerialParserTest, DetectValiddStartAndStopSequence) {

    MessageHandling::SerialParser sp{&m_messageHandler};
    
    EXPECT_CALL(m_messageHandler, loadIncomingMessage).Times(1);
    
    std::deque<uint8_t> chars{};
    chars.push_back('a');
    chars.push_back('b');
    
    for (size_t i = 0; i < START_SEQUENCE_LENGTH; ++i)
        chars.push_back(START_SEQUENCE[i]);
    
    chars.push_back('c');
    chars.push_back('d');
    
    for (size_t i = 0; i < STOP_SEQUENCE_LENGTH; ++i)
        chars.push_back(STOP_SEQUENCE[i]);
        
    // here we call ProcessIncomingData at each char to force the state machine to work across multiple calls
    // rather than processing all incoming data at once
    while (!chars.empty())
    {
        sp.ReceiveChar(chars.front());
        chars.pop_front();
        sp.ProcessIncomingData();
    }
}

TEST_F(SerialParserTest, NoValidStartSequence) {

    MessageHandling::SerialParser sp{&m_messageHandler};
    
    EXPECT_CALL(m_messageHandler, loadIncomingMessage).Times(0);
    
    std::deque<uint8_t> chars{};
    chars.push_back('a');
    chars.push_back('b');
    chars.push_back('c');
    chars.push_back('d');
    chars.push_back('e');
    chars.push_back('f');
    
    for (size_t i = 0; i < STOP_SEQUENCE_LENGTH; ++i)
        chars.push_back(STOP_SEQUENCE[i]);
        
    while (!chars.empty())
    {
        sp.ReceiveChar(chars.front());
        chars.pop_front();
    }
    
    // here we can just process all incoming data in one batch
    sp.ProcessIncomingData();
}

TEST_F(SerialParserTest, NoValidStopSequence) {

    MessageHandling::SerialParser sp{&m_messageHandler};
    
    EXPECT_CALL(m_messageHandler, loadIncomingMessage).Times(0);
    
    std::deque<uint8_t> chars{};
    chars.push_back('a');
    chars.push_back('b');
    chars.push_back('c');
    
    for (size_t i = 0; i < START_SEQUENCE_LENGTH; ++i)
        chars.push_back(START_SEQUENCE[i]);
    
    chars.push_back('d');
    chars.push_back('e');
    chars.push_back('f');
            
    while (!chars.empty())
    {
        sp.ReceiveChar(chars.front());
        chars.pop_front();
    }
    
    // here we can just process all incoming data in one batch   
    sp.ProcessIncomingData();
}

TEST_F(SerialParserTest, Timeout) {

    MessageHandling::SerialParser sp{&m_messageHandler};
    
    EXPECT_CALL(m_messageHandler, loadIncomingMessage).Times(0);
    
    std::deque<uint8_t> chars{};
    chars.push_back('a');
    chars.push_back('b');
    
    for (size_t i = 0; i < START_SEQUENCE_LENGTH; ++i)
        chars.push_back(START_SEQUENCE[i]);
        
    chars.push_back('c');
    chars.push_back('d');
    
    for (size_t i = 0; i < STOP_SEQUENCE_LENGTH; ++i)
        chars.push_back(STOP_SEQUENCE[i]);

    
    // process the first couple of chars prior to delay
    // note we need to start processing the start sequence here, otherwise we'll just
    // throw out the first few and continue processing with the full message after the
    // timeout
    sp.ReceiveChar(chars.front());
    chars.pop_front();
    sp.ReceiveChar(chars.front());
    chars.pop_front();    
    sp.ReceiveChar(chars.front());
    chars.pop_front();    
    sp.ReceiveChar(chars.front());
    chars.pop_front();    
    
    // now delay
    std::this_thread::sleep_for(std::chrono::milliseconds(MessageHandling::SERIAL_TRANSMISSION_TIMEOUT_TICKS+20));

    // now if we try to process the incoming data, we should get a timeout
    sp.ProcessIncomingData();

    // now load the rest of the data        
    while (!chars.empty())
    {
        sp.ReceiveChar(chars.front());
        chars.pop_front();
    }
    
    // and process, this should not trigger a call to loadIncomingMessage since we've timed out and cleared the
    // initial part of the input sequence
    sp.ProcessIncomingData();
}

TEST_F(SerialParserTest, Overflow) {

    MessageHandling::SerialParser sp{&m_messageHandler};
    
    EXPECT_CALL(m_messageHandler, loadIncomingMessage).Times(0);
    
    // load up the start sequence, this puts the state machine into a regime where it will check for maximum message length
    for (size_t i = 0; i < START_SEQUENCE_LENGTH; ++i)
        sp.ReceiveChar(START_SEQUENCE[i]);
        
    sp.ProcessIncomingData();
    
    // now load up characters over the max message length
    for (size_t i = 0; i <= MessageHandling::MAX_INCOMING_MESSAGE_LENGTH; ++i)
        sp.ReceiveChar(i % 256);

    // add the stop sequence so that it would be a valid message if not for the excessive length    
    for (size_t i = 0; i < STOP_SEQUENCE_LENGTH; ++i)
        sp.ReceiveChar(STOP_SEQUENCE[i]);
    
    // attempt to process the data, we should get an overflow and not call the load message function from the message handler
    sp.ProcessIncomingData();
}

