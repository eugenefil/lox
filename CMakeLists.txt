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

find_package(GTest REQUIRED)
enable_testing()
include(GoogleTest)

# put all binaries in the same dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(src)
add_subdirectory(tests)
