#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <TestTimebase.h>
#include <SingletonFactory.h>
#include <TestLogger.h>
#include <IMotorDriver.h>
#include <MockMotorDriver.h>
#include <Controller.h>
#include <MotorControlState.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::_;

class ControllerTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
            SingletonFactory factory;
            std::shared_ptr<ITimebase> pTimebase = factory.create<ITimebase, TestTimebase>();
            std::shared_ptr<Logging::BaseLogger> pLogger = factory.create<Logging::BaseLogger, Logging::TestLogger>();
            
            m_pController = std::make_unique<MotorControl::Controller>(static_cast<MotorControl::IMotorDriver*>(&m_motorDriver));
        }
        
        // ramp the motor to a new setpoint, track the ramp progress, and verify final PWM and state
        // this pattern is used in several tests below
        // this function does not handle zero crossings
        void RampAndCheck(bool forwardDirection, uint16_t dutyCycle, uint16_t rampTime_ms)
        {
            std::cout << "\n\n\nCONTROLLER TEST: RampAndCheck " << (forwardDirection ? "forward" : "reverse") << " direction; dutyCycle = "
                      << dutyCycle << "; rampTime_ms = " << rampTime_ms << "\n";
              
            // ramp direction will be positive or negative, we need to know this to track the change in duty cycle
            // over time
            int initialDutyCycle{static_cast<int>(m_motorDriver.getPwmDutyCycle())};
         
            // when we start the ramp from zero speed, we should set the direction
            if (initialDutyCycle == 0)
                EXPECT_CALL(m_motorDriver, setForwardDirection(forwardDirection)).Times(1);
            
            // record start time
            std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();

            // ramp motor on in set direction
            m_pController->sendCommand(forwardDirection, dutyCycle, rampTime_ms);

            // Now we'll loop over the ramp time. As we call the update function, the ramp
            // should proceed. We will expect calls to the motor driver to update the PWM,
            // and we can check that the PWM value is on schedule based on the ramp time by
            // reading the PWM from the motor driver. At each iteration, we'll check that the
            // current PWM value is near the expected value based on a nominal linear ramp;

            int32_t nominalPwm;
            std::chrono::duration<double, std::milli> elapsedTime_ms{0};   

            // we'll time loop to get an accurate tick rate
            std::chrono::time_point<std::chrono::steady_clock> loopTime;
            float sleepTime_us{0};

            // we'll loop over the ramp time in ms
            while (m_pController->getState() == MotorControl::RAMP)
            {
                loopTime = std::chrono::steady_clock::now();
                // std::cout << "Loop start: " << std::chrono::duration_cast<std::chrono::milliseconds>(loopTime.time_since_epoch()).count() << "\n";
                
                // the nominal PWM value for the linear ramp
                elapsedTime_ms = loopTime - startTime;
                nominalPwm = initialDutyCycle + (dutyCycle - initialDutyCycle)*elapsedTime_ms.count()/rampTime_ms;
                
                std::cout << "Elapsed time " << elapsedTime_ms.count() << "; nominal PWM " << nominalPwm
                          << "; actual PWM " << m_motorDriver.getPwmDutyCycle() << "\n";               
                
                // expect motor driver calls for the controller tick update
                // We call getPwmDutyCycle to verify the correct value, there is also a second call when the ramp finishes
                // and we move to the running state (in the running state constructor)
                // the controller will call setPwmDutyCycle within the tick when it updates the value for the ramp
                EXPECT_CALL(m_motorDriver, getPwmDutyCycle()).Times(AtLeast(1));
                EXPECT_CALL(m_motorDriver, setPwmDutyCycle(_)).Times(1);
                
                // tick
                m_pController->tick();
                
                // verify PWM value within 5% of the limit value
                EXPECT_NEAR(m_motorDriver.getPwmDutyCycle(), nominalPwm, std::max(static_cast<int>(0.2*nominalPwm), 50));
                
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
            EXPECT_NEAR(rampTime_ms, elapsedTime_ms.count(), 5);
            
            // verify final PWM is exactly correct
            EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
            
            // once ramp is finished, we should be in running or idle state
            if (dutyCycle == 0)
                EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
            else
                EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
        }
        
        // ramp the motor to a new setpoint, track the ramp progress, and verify final PWM and state
        // this pattern is used in several tests below
        // this function handles running from forward to reverse and vice versa
        void RampAndCheckWithDirectionChange(uint16_t dutyCycle, uint16_t rampTime_ms)
        {
            // ramp direction will be positive or negative, we need to know this to track the change in duty cycle
            // over time
            int initialDutyCycle{static_cast<int>(m_motorDriver.getPwmDutyCycle())};
        
            // we should not run this function from zero duty cycle
            ASSERT_NE(initialDutyCycle, 0);
        
            std::cout << "\n\n\nCONTROLLER TEST: RampAndCheckWithDirectionChange start direction = " 
                      << (m_motorDriver.getForwardDirection() ? "forward" : "reverse")
                      << " final direction = "
                      << (m_motorDriver.getForwardDirection() ? "reverse" : "forward")
                      << " initial duty cycle = " << initialDutyCycle 
                      << " final duty cycle = " << dutyCycle 
                      << " rampTime_ms = " << rampTime_ms << "\n";
                
            bool isInitialDirectionForward = m_motorDriver.getForwardDirection();
                
            // the duty cycle change rate is equal to the total change divided by the ramp time
            int dutyCycleChangeRate = (dutyCycle + initialDutyCycle)/rampTime_ms;
            
            // at the start, we will reduce the PWM until we change direction
            int dutyCycleChangeDirection = -1;
                
            // record start time
            std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();            

            // send command to start the ramp
            m_pController->sendCommand(!m_motorDriver.getForwardDirection(), dutyCycle, rampTime_ms);

            // Now we'll loop over the ramp time. As we call the update function, the ramp
            // should proceed. We will expect calls to the motor driver to update the PWM,
            // and we can check that the PWM value is on schedule based on the ramp time by
            // reading the PWM from the motor driver. At each iteration, we'll check that the
            // current PWM value is near the expected value based on a nominal linear ramp;
            
            int32_t nominalPwm;
            std::chrono::duration<double, std::milli> elapsedTime_ms{0};   

            // we'll time loop to get an accurate tick rate
            std::chrono::time_point<std::chrono::steady_clock> loopTime;
            float sleepTime_us{0};

            // we'll expect the direction call change to happen at least once
            EXPECT_CALL(m_motorDriver, setForwardDirection(!isInitialDirectionForward)).Times(AtLeast(1));

            // we'll loop over the ramp time in ms
            while (m_pController->getState() == MotorControl::RAMP)
            {
                loopTime = std::chrono::steady_clock::now();
                // std::cout << "Loop start: " << std::chrono::duration_cast<std::chrono::milliseconds>(loopTime.time_since_epoch()).count() << "\n";
                
                // the nominal PWM value for the linear ramp
                elapsedTime_ms = loopTime - startTime;
                nominalPwm = std::abs(initialDutyCycle - (dutyCycle + initialDutyCycle)*elapsedTime_ms.count()/rampTime_ms);
                
                std::cout << "Elapsed time " << elapsedTime_ms.count() << "; nominal PWM " << nominalPwm
                          << "; actual PWM " << m_motorDriver.getPwmDutyCycle() << "\n";
                
                // expect motor driver calls for the controller tick update
                // We call getPwmDutyCycle to verify the correct value, there is also a second call when the ramp finishes
                // and we move to the running state (in the running state constructor)
                // the controller will call setPwmDutyCycle within the tick when it updates the value for the ramp
                EXPECT_CALL(m_motorDriver, getPwmDutyCycle()).Times(AtLeast(1));
                EXPECT_CALL(m_motorDriver, setPwmDutyCycle(_)).Times(1);
                
                // tick
                m_pController->tick();
                
                // verify PWM value within 5% of the limit value
                EXPECT_NEAR(m_motorDriver.getPwmDutyCycle(), nominalPwm, std::max(static_cast<int>(0.2*nominalPwm), 50));
                
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
            EXPECT_NEAR(rampTime_ms, elapsedTime_ms.count(), 5);
            
            // verify final PWM is exactly correct
            EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
            
            // verify final direction
            EXPECT_FALSE(m_motorDriver.getForwardDirection() == isInitialDirectionForward);
            
            // once ramp is finished, we should be in running or idle state
            if (dutyCycle == 0)
                EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
            else
                EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
        }
        
        NiceMock<MotorControl::MockMotorDriver> m_motorDriver{};
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

TEST_F(ControllerTest, RampMotorZeroToForwardToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    bool forwardDirection = true;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(forwardDirection, dutyCycle, rampTime_ms);
    
    // now ramp back to zero
    RampAndCheck(forwardDirection, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToReverseToForwardToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to reverse direction
    RampAndCheck(false, dutyCycle, rampTime_ms);
    
    // now ramp to forward direction
    RampAndCheckWithDirectionChange(dutyCycle, rampTime_ms);
    
    // and ramp back to zero
    RampAndCheck(true, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToForwardToReverseToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, dutyCycle, rampTime_ms);
    
    // now ramp to reverse direction
    RampAndCheckWithDirectionChange(dutyCycle, rampTime_ms);
    
    // and ramp back to zero
    RampAndCheck(true, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToForwardToForwardToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle1 = 500;
    uint16_t dutyCycle2 = 600;
    uint16_t dutyCycle3 = 400;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, dutyCycle1, rampTime_ms);
    
    // execute and validate the ramp from forward to forward direction, increasing duty cycle
    RampAndCheck(true, dutyCycle2, rampTime_ms);
    
    // execute and validate the ramp from forward to forward direction, decreasing duty cycle
    RampAndCheck(true, dutyCycle3, rampTime_ms);
    
    // and ramp back to zero
    RampAndCheck(true, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToReverseToReverseToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle1 = 500;
    uint16_t dutyCycle2 = 600;
    uint16_t dutyCycle3 = 400;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to reverse direction
    RampAndCheck(false, dutyCycle1, rampTime_ms);
    
    // execute and validate the ramp from reverse to reverse direction, increasing duty cycle
    RampAndCheck(false, dutyCycle2, rampTime_ms);
    
    // execute and validate the ramp from reverse to reverse direction, decreasing duty cycle
    RampAndCheck(false, dutyCycle3, rampTime_ms);
    
    // and ramp back to zero
    RampAndCheck(false, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToZero) {

    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // send ramp command with zero duty cycle, which should move the state to ramp
    m_pController->sendCommand(true, 0, rampTime_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to idle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToForwardToForwardNoChangeToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, dutyCycle, rampTime_ms);
    
    // send ramp command with same duty cycle, which should move the state to ramp
    m_pController->sendCommand(true, dutyCycle, rampTime_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running; we do allow for a
    // set duty cycle call here
    EXPECT_CALL(m_motorDriver, setPwmDutyCycle(dutyCycle)).Times(AtMost(1));
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    
    // and ramp back to zero
    RampAndCheck(true, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToReverseToReverseNoChangeToZero) {

    // duty cycle is provided as tenths of a percent of maximum
    uint16_t dutyCycle = 500;
    uint16_t rampTime_ms = 100;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(false, dutyCycle, rampTime_ms);
    
    // send ramp command with same duty cycle, which should move the state to ramp
    m_pController->sendCommand(false, dutyCycle, rampTime_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running; we do allow for a
    // set duty cycle call here
    EXPECT_CALL(m_motorDriver, setPwmDutyCycle(dutyCycle)).Times(AtMost(1));
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    
    // and ramp back to zero
    RampAndCheck(false, 0, rampTime_ms);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToForwardSlowToZero) {

    // these values are chosen such that when we use rampTimeSlow_ms we will below below the minimum change rate
    // of one tenth of a percent per tick
    uint16_t dutyCycle = 100;
    uint16_t rampTimeSlow_ms = 200;
    uint16_t rampTimeFast_ms = 20;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // send ramp command
    m_pController->sendCommand(true, dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to idle, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), 0);
    
    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, dutyCycle, rampTimeFast_ms);
    
    // now try to ramp up too slowly from the running state
    m_pController->sendCommand(true, 2*dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    
    // now try to ramp back to zero too slowly
    m_pController->sendCommand(true, 0, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorZeroToReverseSlowToZero) {

    // these values are chosen such that when we use rampTimeSlow_ms we will below below the minimum change rate
    // of one tenth of a percent per tick
    uint16_t dutyCycle = 100;
    uint16_t rampTimeSlow_ms = 200;
    uint16_t rampTimeFast_ms = 20;
    
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    
    // send ramp command
    m_pController->sendCommand(false, dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to idle, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), 0);
    
    // execute and validate the ramp from zero to reverse direction
    RampAndCheck(false, dutyCycle, rampTimeFast_ms);
    
    // now try to ramp up too slowly from the running state
    m_pController->sendCommand(false, 2*dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    
    // now try to ramp back to zero too slowly
    m_pController->sendCommand(false, 0, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing duty cycle
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorForwardToReverseTooSlow) {

    // these values are chosen such that when we use rampTimeSlow_ms we will below below the minimum change rate
    // of one tenth of a percent per tick
    uint16_t dutyCycle = 100;
    uint16_t rampTimeSlow_ms = 400;
    uint16_t rampTimeFast_ms = 20;

    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);

    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, dutyCycle, rampTimeFast_ms);
    
    // send ramp command to reverse direction, which should move the controller to the ramp state
    m_pController->sendCommand(false, dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing 
    // duty cycle or direction
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    EXPECT_EQ(m_motorDriver.getForwardDirection(), true);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, RampMotorReverseToForwardTooSlow) {

    // these values are chosen such that when we use rampTimeSlow_ms we will below below the minimum change rate
    // of one tenth of a percent per tick
    uint16_t dutyCycle = 100;
    uint16_t rampTimeSlow_ms = 400;
    uint16_t rampTimeFast_ms = 20;

    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);

    // execute and validate the ramp from zero to reverse direction
    RampAndCheck(false, dutyCycle, rampTimeFast_ms);
    
    // send ramp command to reverse direction, which should move the controller to the ramp state
    m_pController->sendCommand(true, dutyCycle, rampTimeSlow_ms);
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // tick the controller, which should move the state from ramp back to running, without changing 
    // duty cycle or direction
    m_pController->tick();
    EXPECT_EQ(m_pController->getState(), MotorControl::RUNNING);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), dutyCycle);
    EXPECT_EQ(m_motorDriver.getForwardDirection(), false);
    
    // EXPECT_TRUE(false);
}

TEST_F(ControllerTest, IdleMotor) {
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);

    // execute and validate the ramp from zero to forward direction
    RampAndCheck(true, 200, 50);
    
    // idle motor call should kick off a ramp
    m_pController->idleMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::RAMP);
    
    // and we should then ramp down
    while (m_pController->getState() == MotorControl::RAMP)
    {
        EXPECT_CALL(m_motorDriver, setPwmDutyCycle(_)).Times(1);
        m_pController->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // verify we're in the idle state
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);
}

TEST_F(ControllerTest, InjectFaultAndClear) {
    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();
    
    // enable motor and move to idle state
    m_pController->enableMotor();
    EXPECT_EQ(m_pController->getState(), MotorControl::IDLE);

    // execute and validate the ramp from zero to reverse direction
    RampAndCheck(true, 200, 50);
    
    // now inject a fault
    m_pController->injectFault();
    
    // confirm we entered the fault state
    EXPECT_EQ(m_pController->getState(), MotorControl::FAULT);
    EXPECT_EQ(m_motorDriver.getPwmDutyCycle(), 0);
    
    // now attempt to clear the fault
    EXPECT_CALL(m_motorDriver, isDriverFaulted()).Times(1).WillOnce(Return(false));
    m_pController->clearFault();
    EXPECT_EQ(m_pController->getState(), MotorControl::DISABLED);
}


TEST_F(ControllerTest, MotorSpeedData) {
    // number of encoder pulses to elapse per tick
    int encoderValue{0};
    int encoderPulsesPerTick{30};

    // delegate calls to the fake motor driver object (keeps track of PWM and direction settings)
    m_motorDriver.DelegateToFake();

    for (size_t i = 0; i < MotorControl::VELOCITY_BUFFER_SIZE; ++i)
    {
        EXPECT_CALL(m_motorDriver, getEncoderCounts()).Times(1).WillOnce(Return(encoderValue));
        encoderValue += encoderPulsesPerTick;
        m_pController->tick();
    }
    
    EXPECT_NEAR(m_pController->getMotorSpeed(), encoderPulsesPerTick, 1);
    
    for (size_t i = 0; i < MotorControl::VELOCITY_BUFFER_SIZE; ++i)
    {
        EXPECT_CALL(m_motorDriver, getEncoderCounts()).Times(1).WillOnce(Return(encoderValue));
        encoderValue -= encoderPulsesPerTick;
        m_pController->tick();
    }
   
    EXPECT_NEAR(m_pController->getMotorSpeed(), -1*encoderPulsesPerTick, 1);
}

TEST_F(ControllerTest, ControllerStateTransitions) {

}

// TODO: Rewrite code for ramp tick, instead of just adding the duty cycle change rate at each tick, instead calculate time from ramp start
// and multiply that by the change rate, and add to the original PWM rate (need to save that); need to redesign how we handle direction
// change in that case

/*  Ramp tests
    [x] ramp from zero to forward direction
    [x] ramp from forward to zero
    [x] ramp from zero to reverse direction
    [x] ramp from reverse to forward direction
    [x] ramp from forward to reverse direction
    [x] ramp from reverse to zero
    [x] ramp from forward to forward
    [x] ramp from reverse to reverse
    [x] ramp from zero to zero (verify we just return to idle right away)
    [x] ramp from forward to forward at same duty cycle, e.g. a change rate of zero (verify this is handled OK)
    [x] ramp from reverse to reverse at same duty cycle, e.g. a change rate of zero (verify this is handled OK)
    [x] ramp from zero to forward with very low rate of change (less than one per controller tick)
    [x] ramp from zero to reverse with very low rate of change (less than one per controller tick)
    [x] ramp from forward to reverse with very low rate of change (less than one per controller tick)
    [x] ramp from reverse to forward with very low rate of change (less than one per controller tick)
    
    [x] NEED TO ADD PROTECTION IF DUTY CYCLE CHANGE RATE IS ZERO (E.G. IF TOTAL DUTY CYCLE CHANGE DIVIDED BY
    RAMP TIME TRUNCATES TO ZERO)
    
    [x] Update logic to fail the motor command if the change rate is too low
    [x] What about if we pass the check with the raw duty cycle, but then it gets clamped, and change rate
        is less than expected?
          - Validate the duty cycle with the motor driver
    
    [x] REFACTOR HOW WE DO DUTY CYCLE IN G2 DRIVER
        - Set and store the max as a percent of the limit (clamp at 1000)
        - Set and store the current value as a percent of the limit
        - When setting, clamp the value to the max
        - Convert from percent to ticks in the updateMotorDrive function
*/

// TESTS:
// [x] Test ramp on/off timing, states, and driver calls
// [x] idleMotor call
// [x] Inject & clear fault
// [x] Call tick function - inject dummy speed data, verify speed calculation and reporting
// [-] Event handling by state - not going to do this, this is too low level, if the above tests are passing that is sufficient
