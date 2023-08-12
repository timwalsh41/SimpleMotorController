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
