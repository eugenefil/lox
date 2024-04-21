#include "Parser.h"
#include <gtest/gtest.h>

using Type = Lox::Token::Type;

void assert_expr(std::vector<Lox::Token>&& tokens, std::string_view ast_repr)
{
    auto ast = Lox::Parser(std::move(tokens)).parse();
    ASSERT_TRUE(ast);
    ASSERT_EQ(ast->dump(), ast_repr);
}

TEST(Parser, LiteralExpressions)
{
    assert_expr({ { Type::String, "", "" } }, R"("")");
    assert_expr({ { Type::String, "", "Hello world!" } }, R"("Hello world!")");
    assert_expr({ { Type::String, "", "\t\r\n\"\\" } }, R"("\t\r\n\"\\")");

    assert_expr({ { Type::Number, "", 123.0 } }, "123");
    assert_expr({ { Type::Number, "", -123.0 } }, "-123");
    assert_expr({ { Type::Number, "", 3.14159265 } }, "3.14159265");

    assert_expr({ { Type::True, "", true } }, "true");
    assert_expr({ { Type::False, "", false } }, "false");
}
