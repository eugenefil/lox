#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

void assert_expr(std::vector<Lox::Token>&& tokens, std::string_view ast_repr)
{
    auto ast = Lox::Parser(std::move(tokens)).parse();
    ASSERT_TRUE(ast);
    ASSERT_EQ(ast->dump(), ast_repr);
}

TEST(Parser, LiteralExpressions)
{
    assert_expr({ { TokenType::String, {}, "" } }, R"("")");
    assert_expr({ { TokenType::String, {}, "Hello world!" } }, R"("Hello world!")");
    assert_expr({ { TokenType::String, {}, "\t\r\n\"\\" } }, R"("\t\r\n\"\\")");

    assert_expr({ { TokenType::Number, {}, 123.0 } }, "123");
    assert_expr({ { TokenType::Number, {}, -123.0 } }, "-123");
    assert_expr({ { TokenType::Number, {}, 3.14159265 } }, "3.14159265");

    assert_expr({ { TokenType::True, {}, true } }, "true");
    assert_expr({ { TokenType::False, {}, false } }, "false");

    assert_expr({ { TokenType::Nil, {} } }, "nil");
}

TEST(Parser, UnaryExpressions)
{
    assert_expr({ { TokenType::Minus, {} }, { TokenType::Number, {}, 1.0 } }, "(- 1)");
}
