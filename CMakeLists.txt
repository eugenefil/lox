cmake_minimum_required(VERSION 3.23)

project(lox
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION "Lox language"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_COLOR_DIAGNOSTICS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(lib Utils.cpp Lexer.cpp AST.cpp Parser.cpp Interpreter.cpp)

add_executable(lox main.cpp)
target_link_libraries(lox lib)

find_package(GTest REQUIRED)
enable_testing()
include(GoogleTest)

function(lox_test source)
    get_filename_component(test_name ${source} NAME_WE)
    add_executable(${test_name} ${source})
    target_link_libraries(${test_name} lib GTest::gtest_main)
    gtest_discover_tests(${test_name})
endfunction()

lox_test(TestLexer.cpp)
lox_test(TestParser.cpp)
lox_test(TestInterpreter.cpp)
