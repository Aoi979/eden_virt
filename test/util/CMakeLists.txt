
add_executable(util_test util_test.cpp)
target_link_libraries(util_test gtest_main virtlib)
include(GoogleTest)
gtest_discover_tests(util_test)