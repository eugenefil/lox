#include "Parser.h"
#include <gtest/gtest.h>

using Type = Lox::Token::Type;

void assert_expr(std::vector<Lox::Token>&& tokens, std::string_view ast_repr)
{
    auto ast = Lox::Parser(std::move(tokens)).parse();
    ASSERT_TRUE(ast);
    ASSERT_EQ(ast->dump(), ast_repr);
}

TEST(Parser, StringLiteral)
{
    assert_expr({ { Type::String, "", "" } }, R"("")");
    assert_expr({ { Type::String, "", "foo bar" } }, R"("foo bar")");
    assert_expr({ { Type::String, "", "\t\r\n\"\\" } }, R"("\t\r\n\"\\")");
}
