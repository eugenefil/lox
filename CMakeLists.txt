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

add_library(lib Scanner.cpp)

add_executable(lox main.cpp)
target_link_libraries(lox lib)

find_package(GTest REQUIRED)
enable_testing()
add_executable(TestScanner TestScanner.cpp)
target_link_libraries(TestScanner lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(TestScanner)
