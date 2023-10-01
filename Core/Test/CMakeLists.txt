include(GoogleTest)

#********** MESSAGE TEST ***********************

add_executable(
  MessageTest
  MessageTest.cpp
  MockLogger.cpp
  ../Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
)
target_link_libraries(
  MessageTest
  GTest::gtest_main
)

target_include_directories(MessageTest PUBLIC ../Inc/ ../Inc/Control)

gtest_discover_tests(MessageTest)



#********** CURRENT SENSOR TEST *****************

add_executable(
  CurrentSensorTest
  CurrentSensorTest.cpp
  MockLogger.cpp
  ../Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
  ../Src/CurrentSensor.cpp
)

target_link_libraries(
  CurrentSensorTest
  GTest::gtest_main
)

target_include_directories(CurrentSensorTest PUBLIC ../Inc/ ../Inc/Control)

gtest_discover_tests(CurrentSensorTest)



#********* SERIAL PARSER TEST *****************

add_executable(
  SerialParserTest
  SerialParserTest.cpp
  MockLogger.cpp
  ../Src/SingletonFactory.cpp
  ../Src/TestTimebase.cpp
  ../Src/Message.cpp
  ../Src/SerialParser.cpp
)

target_link_libraries(
  SerialParserTest
  GTest::gtest_main
  GTest::gmock_main
)

target_include_directories(SerialParserTest PUBLIC 
    ../Inc/ 
    ../Inc/Control 
    ../../Drivers/STM32L4xx_HAL_Driver/Inc 
    ../../Drivers/STM32L4xx_HAL_Driver/Inc/Legacy
    ../../Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ../../Drivers/CMSIS/Include
    )

gtest_discover_tests(SerialParserTest)


#******** CONTROLLER TEST *******************

add_executable(
  ControllerTest
  ControllerTest.cpp
  MockLogger.cpp
  ../Src/SingletonFactory.cpp
  ../Src/TestTimebase.cpp
  ../Src/Control/Controller.cpp
  ../Src/Control/MotorControlState.cpp
  ../Src/Utility.cpp
)

target_link_libraries(
  ControllerTest
  GTest::gtest_main
  GTest::gmock_main
)

target_include_directories(ControllerTest PUBLIC 
    ../Inc/ 
    ../Inc/Control 
    )

gtest_discover_tests(ControllerTest)


#******** MESSAGEHANDLER TEST *******************

add_executable(
  MessageHandlerTest
  MessageHandlerTest.cpp
  MockLogger.cpp
  ../Src/SingletonFactory.cpp
  ../Src/TestTimebase.cpp
  ../Src/MessageHandler.cpp
  ../Src/Message.cpp
  ../Src/CurrentSensor.cpp
  ../Src/Version.cpp
)

target_link_libraries(
  MessageHandlerTest
  GTest::gtest_main
  GTest::gmock_main
)

target_include_directories(MessageHandlerTest PUBLIC 
    ../Inc/ 
    ../Inc/Control 
    )

gtest_discover_tests(MessageHandlerTest)
