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

