#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <TestTimebase.h>
#include <SingletonFactory.h>
#include <TestLogger.h>
#include <IMotorCommandHandler.h>
#include <MockMotorCommandHandler.h>
#include <MockOutputGateway.h>
#include <MessageHandler.h>
#include <CommandInterface.h>
#include <Version.h>
#include <iostream>
#include <memory>

using ::testing::Return;

class MessageHandlerTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {                  
            m_pTimebase = m_factory.create<ITimebase, TestTimebase>();
            m_pLogger = m_factory.create<Logging::BaseLogger, Logging::TestLogger>();
            m_pCurrentSensor = m_factory.create<Sensors::CurrentSensor>();
            m_pMotorCommandHandler = std::static_pointer_cast<MockMotorCommandHandler>(m_factory.create<IMotorCommandHandler, MockMotorCommandHandler>());
            m_pOutputGateway = std::static_pointer_cast<MockOutputGateway>(m_factory.create<IOutputGateway, MockOutputGateway>());
            
            m_pMessageHandler = std::make_unique<MessageHandling::MessageHandler>();
            
            std::cout << "Output gateway is at " << m_pOutputGateway.get() << "\n";
        }
        
        void TearDown() override
        {
            std::cout << "Tear down function\n";

            // IMPORTANT: we need to clear the singletons map or else we get a seg fault due
            // to conflicting lifetimes/references between gtest/gmock objects and the static
            // map of singletons held by the SingletonFactory class
            m_factory.clearSingletons();
        }
        
        SingletonFactory m_factory;
        std::shared_ptr<MockOutputGateway> m_pOutputGateway;
        std::shared_ptr<ITimebase> m_pTimebase;
        std::shared_ptr<Logging::BaseLogger> m_pLogger;
        std::shared_ptr<Sensors::CurrentSensor> m_pCurrentSensor;
        std::shared_ptr<MockMotorCommandHandler> m_pMotorCommandHandler;
        std::unique_ptr<MessageHandling::MessageHandler> m_pMessageHandler;
};

TEST_F(MessageHandlerTest, InvalidCrcMessage) {
    MessageHandling::Message msg;    
    
    // m_pOutputGateway is a shared pointer to IOutputGateway, we need to cast to MockOutputGateway to use some of
    // the fake functionality
    // std::cout << "Casting output gateway pointer to MockOutputGateway\n";
    // std::shared_ptr<MockOutputGateway> pOutputGateway = std::static_pointer_cast<MockOutputGateway>(m_pOutputGateway);
    
    // Because our mock output gateway is held outside of this function, it will leak (e.g. persist) after the
    // test function is complete. For that reason, we will allow it to leak, and manually call VerifyAndClearExpectations
    // below to confirm that expectations/calls are met.
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // ::testing::Mock::AllowLeak(m_pOutputGateway.get());
    
    m_pOutputGateway->DelegateToFake();

    // pick a dummy command code and then set CRC
    msg.setCommandCode(0xAA);
    msg.setCRC();
    
    // now change command code, which will invalidate CRC
    msg.setCommandCode(0xFF);
    
    // now load and process the message
    EXPECT_CALL(*m_pOutputGateway, sendMessage).Times(1);
    EXPECT_CALL(*m_pOutputGateway, processMessage()).Times(1);
    
    m_pMessageHandler->loadIncomingMessage(msg);    
    m_pMessageHandler->processMessages();
    
    EXPECT_EQ(m_pOutputGateway->processMessage(), ACK_INVALID_CRC);
    
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(m_pOutputGateway.get()));
    
    // ASSERT_TRUE(false);
}

TEST_F(MessageHandlerTest, InvalidCommandCodeMessage) {
    MessageHandling::Message msg;    
    
    // m_pOutputGateway is a shared pointer to IOutputGateway, we need to cast to MockOutputGateway to use some of
    // the fake functionality
    // std::cout << "Casting output gateway pointer to MockOutputGateway\n";
    // std::shared_ptr<MockOutputGateway> pOutputGateway = std::static_pointer_cast<MockOutputGateway>(m_pOutputGateway);
    
    // Because our mock output gateway is held outside of this function, it will leak (e.g. persist) after the
    // test function is complete. For that reason, we will allow it to leak, and manually call VerifyAndClearExpectations
    // below to confirm that expectations/calls are met.
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // ::testing::Mock::AllowLeak(m_pOutputGateway.get());
    
    m_pOutputGateway->DelegateToFake();

    // pick an invalid command code and then set CRC
    msg.setCommandCode(0x00);
    msg.setCRC();
    
    // now load and process the message
    EXPECT_CALL(*m_pOutputGateway, sendMessage).Times(1);
    EXPECT_CALL(*m_pOutputGateway, processMessage()).Times(1);
    
    m_pMessageHandler->loadIncomingMessage(msg);    
    m_pMessageHandler->processMessages();
    
    EXPECT_EQ(m_pOutputGateway->processMessage(), ACK_INVALID_COMMAND);
    
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(m_pOutputGateway.get()));
    
    // ASSERT_TRUE(false);
}

TEST_F(MessageHandlerTest, CommandExecutionFailure) {
    MessageHandling::Message msg;    
    
    // m_pOutputGateway is a shared pointer to IOutputGateway, we need to cast to MockOutputGateway to use some of
    // the fake functionality
    // std::cout << "Casting output gateway pointer to MockOutputGateway\n";
    // std::shared_ptr<MockOutputGateway> pOutputGateway = std::static_pointer_cast<MockOutputGateway>(m_pOutputGateway);
    
    // Because our mock output gateway is held outside of this function, it will leak (e.g. persist) after the
    // test function is complete. For that reason, we will allow it to leak, and manually call VerifyAndClearExpectations
    // below to confirm that expectations/calls are met.
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // ::testing::Mock::AllowLeak(m_pOutputGateway.get());
    
    m_pOutputGateway->DelegateToFake();

    // pick an invalid command code and then set CRC
    msg.setCommandCode(CMD_ENABLE_MOTOR);
    msg.setCRC();
    
    // now load and process the message
    EXPECT_CALL(*m_pOutputGateway, sendMessage).Times(1);
    EXPECT_CALL(*m_pOutputGateway, processMessage()).Times(1);
    EXPECT_CALL(*m_pMotorCommandHandler, enableMotor).Times(1).WillOnce(Return(false));
    
    m_pMessageHandler->loadIncomingMessage(msg);    
    m_pMessageHandler->processMessages();
    
    EXPECT_EQ(m_pOutputGateway->processMessage(), ACK_COMMAND_ERROR);
    
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(m_pOutputGateway.get()));
    
    // ASSERT_TRUE(false);
}

TEST_F(MessageHandlerTest, CommandExecutionSucccess) {
    MessageHandling::Message msg;    
    
    // m_pOutputGateway is a shared pointer to IOutputGateway, we need to cast to MockOutputGateway to use some of
    // the fake functionality
    // std::cout << "Casting output gateway pointer to MockOutputGateway\n";
    // std::shared_ptr<MockOutputGateway> pOutputGateway = std::static_pointer_cast<MockOutputGateway>(m_pOutputGateway);
    
    // Because our mock output gateway is held outside of this function, it will leak (e.g. persist) after the
    // test function is complete. For that reason, we will allow it to leak, and manually call VerifyAndClearExpectations
    // below to confirm that expectations/calls are met.
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // ::testing::Mock::AllowLeak(m_pOutputGateway.get());
    
    m_pOutputGateway->DelegateToFake();

    // pick an invalid command code and then set CRC
    msg.setCommandCode(CMD_ENABLE_MOTOR);
    msg.setCRC();
    
    // now load and process the message
    EXPECT_CALL(*m_pOutputGateway, sendMessage).Times(1);
    EXPECT_CALL(*m_pOutputGateway, processMessage()).Times(1);
    EXPECT_CALL(*m_pMotorCommandHandler, enableMotor).Times(1).WillOnce(Return(true));
    
    m_pMessageHandler->loadIncomingMessage(msg);    
    m_pMessageHandler->processMessages();
    
    EXPECT_EQ(m_pOutputGateway->processMessage(), ACK_SUCCESSFUL);
    
    // NOTE - THIS IS ACTUALLY NOT NEEDED ONCE WE CLEAR THE SINGLETON MAP IN THE TEARDOWN FUNCTION
    // (leaving this comments for posterity)
    // EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(m_pOutputGateway.get()));
    
    // ASSERT_TRUE(false);
}

// TODO:
// - Troubleshoot issues with mock output gateway
// - Remove all the <iostream> & cout from the production code

/*
[x] Build a mock/fake output gateway - we want to inspect the outgoing message
[x] Rename logging classes (ILogger becomes BaseLogger, Logger becomes RealLogger, MockLogger
    becomes TestLogger)

Tests:
[x] Invalid CRC message
[x] Invalid command code message
[x] Unable to execute command - need fake command handler
[ ] Successful command execution

*/
