add_library(LibLox Utils.cpp Lexer.cpp AST.cpp Parser.cpp Interpreter.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED IMPORTED_TARGET readline)

add_executable(lox main.cpp Prelude.cpp)
target_link_libraries(lox LibLox PkgConfig::READLINE)

function(lox_test source)
    get_filename_component(test_name ${source} NAME_WE)
    add_executable(${test_name} ${source})
    target_link_libraries(${test_name} LibLox GTest::gtest_main)
    gtest_discover_tests(${test_name})
endfunction()

lox_test(TestUtils.cpp)
lox_test(TestInterpreter.cpp)
