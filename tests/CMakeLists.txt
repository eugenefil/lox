# runs functional tests for lexer, parser and interpreter
add_executable(TestRunner TestRunner.cpp)
target_link_libraries(TestRunner GTest::gtest) # don't use gtest main
# start TestRunner in the tests dir
gtest_discover_tests(TestRunner WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

