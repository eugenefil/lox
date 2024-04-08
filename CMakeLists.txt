cmake_minimum_required(VERSION 3.23)

project(lox
    VERSION 0.0.1
    LANGUAGES CXX
    DESCRIPTION "Lox language"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(lox main.cpp)
