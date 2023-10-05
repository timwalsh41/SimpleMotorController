include(GoogleTest)

#********** MESSAGE TEST ***********************

add_executable(
  MessageTest
  ./Src/MessageTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
)
target_link_libraries(
  MessageTest
  GTest::gtest_main
)

target_include_directories(MessageTest PUBLIC ./Inc/ ../Inc/ ../Inc/Control/)

gtest_discover_tests(MessageTest)



#********** CURRENT SENSOR TEST *****************

add_executable(
  CurrentSensorTest
  ./Src/CurrentSensorTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
  ../Src/CurrentSensor.cpp
)

target_link_libraries(
  CurrentSensorTest
  GTest::gtest_main
)

target_include_directories(CurrentSensorTest PUBLIC ./Inc/ ../Inc/ ../Inc/Control/)

gtest_discover_tests(CurrentSensorTest)



#********* SERIAL PARSER TEST *****************

add_executable(
  SerialParserTest
  ./Src/SerialParserTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
  ../Src/SerialParser.cpp
)

target_link_libraries(
  SerialParserTest
  GTest::gtest_main
  GTest::gmock_main
)

target_include_directories(SerialParserTest PUBLIC
    ./Inc/
    ../Inc/ 
    ../Inc/Control/
    )

gtest_discover_tests(SerialParserTest)


#******** CONTROLLER TEST *******************

add_executable(
  ControllerTest
  ./Src/ControllerTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
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
    ./Inc/
    ../Inc/ 
    ../Inc/Control/
    )

gtest_discover_tests(ControllerTest)


#******** MESSAGEHANDLER TEST *******************

add_executable(
  MessageHandlerTest
  ./Src/MessageHandlerTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/SingletonFactory.cpp
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
    ./Inc/
    ../Inc/ 
    ../Inc/Control/
    )

gtest_discover_tests(MessageHandlerTest)


#******** OUTPUTGATEWAY TEST *******************

add_executable(
  OutputGatewayTest
  ./Src/OutputGatewayTest.cpp
  ./Src/TestLogger.cpp
  ./Src/TestTimebase.cpp
  ../Src/OutputGateway.cpp
  ../Src/Utility.cpp
  ../Src/SingletonFactory.cpp
  ../Src/Message.cpp
)

target_link_libraries(
  OutputGatewayTest
  GTest::gtest_main
  GTest::gmock_main
)

target_include_directories(OutputGatewayTest PUBLIC
    ./Inc/
    ../Inc/ 
    ../Inc/Control/
    )

gtest_discover_tests(OutputGatewayTest)
