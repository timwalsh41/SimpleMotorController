#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <TestTimebase.h>
#include <TestLogger.h>
#include <IUartWrapper.h>
#include <MockUartWrapper.h>
#include <SingletonFactory.h>
#include <Message.h>
#include <OutputGateway.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class OutputGatewayTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {                  
            m_pTimebase = m_factory.create<ITimebase, TestTimebase>();
            m_pLogger = m_factory.create<Logging::BaseLogger, Logging::TestLogger>();
            m_outputGateway.setReceiving();
        }
        
        SingletonFactory m_factory;
        std::shared_ptr<ITimebase> m_pTimebase;
        std::shared_ptr<Logging::BaseLogger> m_pLogger;
        
        MockUartWrapper m_uartWrapper;
        MessageHandling::OutputGateway m_outputGateway{static_cast<IUartWrapper*>(&m_uartWrapper)};
};

TEST_F(OutputGatewayTest, sendMessage) {
    MessageHandling::Message msg;
    std::vector<uint8_t> payload{'A', 'B', 'C', 'D', 'E'};
    std::string expectedOutputString{};
    
    // the output gateway is purely a pass through for messages, all checking occurs
    // in other objects, so we don't need to create a valid message here, just verify
    // that it is correctly transformed into a string with appropriate headers

    msg.setCommandCode(CMD_RAMP_MOTOR_ON);
    msg.setPayload(payload);
    msg.setCRC();

    // manually assemble the output string to be transmitted
    expectedOutputString.append(START_SEQUENCE, START_SEQUENCE_LENGTH);
    expectedOutputString += static_cast<char>(msg.getCommandCode());
    expectedOutputString.append(reinterpret_cast<char*>(payload.data()), payload.size());

    expectedOutputString += static_cast<uint8_t>(msg.getCRC());
    expectedOutputString += static_cast<uint8_t>(msg.getCRC() >> 8);
    expectedOutputString.append(STOP_SEQUENCE, STOP_SEQUENCE_LENGTH);

    m_outputGateway.sendMessage(msg);

    EXPECT_CALL(m_uartWrapper, transmitString(::testing::StrEq(expectedOutputString))).Times(1);
    m_outputGateway.processMessages();
}
