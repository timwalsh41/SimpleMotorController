include(GoogleTest)

add_executable(
  MessageTest
  MessageTest.cpp
  MockLogger.cpp
  ../Src/Message.cpp
)
target_link_libraries(
  MessageTest
  GTest::gtest_main
)

target_include_directories(MessageTest PUBLIC ../Inc/ ../Inc/Control)

gtest_discover_tests(MessageTest)

add_executable(
  CurrentSensorTest
  CurrentSensorTest.cpp
  MockLogger.cpp
  ../Src/Message.cpp
  ../Src/CurrentSensor.cpp
)

target_link_libraries(
  CurrentSensorTest
  GTest::gtest_main
)

target_include_directories(CurrentSensorTest PUBLIC ../Inc/ ../Inc/Control)

gtest_discover_tests(CurrentSensorTest)

add_executable(
  SerialParserTest
  SerialParserTest.cpp
  MockLogger.cpp
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

