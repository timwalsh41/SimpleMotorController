#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <TestTimebase.h>
#include <SingletonFactory.h>
#include <ILogger.h>
#include <MockLogger.h>
#include <Control/IMotorDriver.h>
#include <Control/MockMotorDriver.h>
#include <Control/Controller.h>
#include <Control/MotorControlState.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::_;

class ControllerTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            SingletonFactory factory;
            std::shared_ptr<ITimebase> pTimebase = factory.create<ITimebase, TestTimebase>();
            std::shared_ptr<Logging::ILogger> pLogger = factory.create<Logging::ILogger, Logging::MockLogger>();
            
            m_pController = std::make_unique<MotorControl::Controller>(static_cast<IMotorDriver*>(&m_motorDriver));
        }
        
        NiceMock<MockMotorDriver> m_motorDriver{};
        std::unique_ptr<MotorControl::Controller> m_pController{};
};

TEST_F(ControllerTest, EnableAndDisableMotor) {

    // motor controller stars up in disabled state, let's verify that
    EXPECT_EQ(m_pController->getState(), MotorControl::DISABLED);

    // first part: enable motor
    // expect call to motor driver to enable motor
    EXPECT_CALL(m_motorDriver, enableMotor()).Times(1);

    // now enable
    m_pController->enableMotor();

    // verify we are in the IDLE state
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);

    // disabling motor
    m_pController->disableMotor();
    
    // verify we are back in disabled state
    EXPECT_EQ(m_pController->getState(), MotorControl::DISABLED);
}

TEST_F(ControllerTest, SleepAndWakeMotor) {
    
    // verify we're in disabled state
    EXPECT_EQ(m_pController->getState(), MotorControl::DISABLED);
   
    // we should call the sleep function of the motor driver
    EXPECT_CALL(m_motorDriver, sleepMotor()).Times(1);
    
    // execute sleep command
    m_pController->sleepDriver();
    
    // verify we're in the sleep state
    EXPECT_EQ(m_pController->getState(), MotorControl::SLEEPING);
    
    // now wake up
    EXPECT_CALL(m_motorDriver, wakeMotor()).Times(1);
    m_pController->wakeDriver();
    
    // verify we return to disabled state
    EXPECT_EQ(m_pController->getState(), MotorControl::DISABLED);
    
    // now enable motor to get to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // and enter sleep mode from idle
    EXPECT_CALL(m_motorDriver, sleepMotor()).Times(1);
    m_pController->sleepDriver();
    EXPECT_EQ(m_pController->getState(), MotorControl::SLEEPING);   
}

TEST_F(ControllerTest, RampMotorZeroToForward) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    bool forwardDirection = true;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // when we start the ramp from zero speed, we should set the direction
    EXPECT_CALL(m_motorDriver, setForwardDirection(forwardDirection)).Times(1);
    
    // record start time
    std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();

    // ramp motor on in forward direction
    m_pController->sendCommand(true, dutyCycle, rampTime_ms);

    // Now we'll loop over the ramp time. As we call the update function, the ramp
    // should proceed. We will expect calls to the motor driver to update the PWM,
    // and we can check that the PWM value is on schedule based on the ramp time by
    // reading the PWM from the motor driver. At each iteration, we'll check that the
    // current PWM value is near the expected value based on a nominal linear ramp;

    uint32_t nominalPwm{0};
    std::chrono::duration<double, std::milli> elapsedTime_ms{0};   

    // we'll time loop to get an accurate tick rate
    std::chrono::time_point<std::chrono::steady_clock> loopTime;
    float sleepTime_us{0};

    // we'll loop over the ramp time in ms
    while (m_pController->getState() == MotorControl::RAMP)
    {
        loopTime = std::chrono::steady_clock::now();
        std::cout << "Loop start: " << std::chrono::duration_cast<std::chrono::milliseconds>(loopTime.time_since_epoch()).count() << "\n";
        
        // the nominal PWM value for the linear ramp
        elapsedTime_ms = loopTime - startTime;
        nominalPwm = std::min(static_cast<uint16_t>(dutyCycle*elapsedTime_ms.count()/rampTime_ms), dutyCycle);
        
        // expect motor driver calls for the controller tick update
        // We call getPwmDutyCycle to verify the correct value, there is also a second call when the ramp finishes
        // and we move to the running state (in the running state constructor)
        // the controller will call setPwmDutyCycle within the tick when it updates the value for the ramp
        EXPECT_CALL(m_motorDriver, getPwmDutyCycle()).Times(AtLeast(1));
        EXPECT_CALL(m_motorDriver, setPwmDutyCycle(_)).Times(1);
        
        // tick
        m_pController->tick();
        
        // verify PWM value
        EXPECT_NEAR(m_motorDriver.getPwmDutyCycle(), nominalPwm, std::max(static_cast<int>(0.2*nominalPwm), 10));
        
        // note that update is run every system tick, which as of the writing of this test
        // is 1 ms
        std::chrono::duration<float, std::micro> elapsedLoopTime = std::chrono::steady_clock::now() - loopTime;
        sleepTime_us = 1000 - elapsedLoopTime.count();
        std::cout << "Sleeping for " << sleepTime_us << " us\n";
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(sleepTime_us)));
    }

    // get finish time
    std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
    
    // get duration of ramp in milliseconds
    elapsedTime_ms = endTime - startTime;
    std::cout << "Total elapsed ramp time is " << elapsedTime_ms.count() << " ms\n";
    EXPECT_NEAR(rampTime_ms, elapsedTime_ms.count(), 2);
    
    // verify final PWM is exactly correct
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    
    // once ramp is finished, we should be in running state
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    
    // EXPECT_TRUE(false);
}

// TODO: Rewrite code for ramp tick, instead of just adding the duty cycle change rate at each tick, instead calculate time from ramp start
// and multiply that by the change rate, and add to the original PWM rate (need to save that); need to redesign how we handle direction
// change in that case

/*  Ramp tests
    - ramp from zero to forward direction
    - ramp from forward to zero
    - ramp from zero to reverse direction
    - ramp from reverse to forward direction
    - ramp from forward to reverse direction
    - ramp from reverse to zero
    - ramp from zero to zero (verify we just return to idle right away)
    - ramp from forward to forward at same duty cycle (verify this is handled OK)
    - ramp from zero to forward
    
    NEED TO ADD PROTECTION IF DUTY CYCLE CHANGE RATE IS ZERO (E.G. IF TOTAL DUTY CYCLE CHANGE DIVIDED BY
    RAMP TIME TRUNCATES TO ZERO)
*/

// TESTS:
// - Test ramp on/off timing, idleMotor call, states, and driver calls
// - Inject fault
// - Set/get max duty cycle 
// - Duty cycle limits
// - Call tick function - inject dummy speed data, verify speed calculation and reporting

