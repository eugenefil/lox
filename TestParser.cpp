#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

void assert_expr(std::vector<Lox::Token> tokens, std::string_view ast_repr)
{
    auto ast = Lox::Parser(std::move(tokens)).parse();
    ASSERT_TRUE(ast);
    ASSERT_EQ(ast->dump(), ast_repr);
}

void assert_errors(std::vector<Lox::Token> tokens, std::vector<Lox::Error> errors)
{
    Lox::Parser parser(std::move(tokens));
    parser.parse();
    auto& errs = parser.errors();
    ASSERT_EQ(errs.size(), errors.size());
    for (std::size_t i = 0; i < errs.size(); ++i)
        EXPECT_EQ(errs[i].span, errors[i].span);
}

TEST(Parser, PrimaryExpressions)
{
    assert_expr({ { TokenType::String, "", "" } }, R"("")");
    assert_expr({ { TokenType::String, "", "Hello world!" } }, R"("Hello world!")");
    assert_expr({ { TokenType::String, "", "\t\r\n\"\\" } }, R"("\t\r\n\"\\")");

    assert_expr({ { TokenType::Number, "", 123.0 } }, "123");
    assert_expr({ { TokenType::Number, "", -123.0 } }, "-123");
    assert_expr({ { TokenType::Number, "", 3.14159265 } }, "3.14159265");

    assert_expr({ { TokenType::Identifier, "foo" } }, "foo");

    assert_expr({ { TokenType::True, "", true } }, "true");
    assert_expr({ { TokenType::False, "", false } }, "false");

    assert_expr({ { TokenType::Nil, "" } }, "nil");

    assert_errors({ { TokenType::Slash, "/" } }, { { "/", "" } });
}

TEST(Parser, UnaryExpressions)
{
    assert_expr({ { TokenType::Minus, "" }, { TokenType::Number, "", 1.0 } }, "(- 1)");
}
