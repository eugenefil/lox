#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

static void assert_expr(std::vector<Lox::Token>&& tokens, std::string_view ast_repr)
{
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    ASSERT_TRUE(ast);
    EXPECT_EQ(ast->dump(0), ast_repr);
}

static void assert_sexp(std::vector<Lox::Token>&& tokens, std::string_view ast_repr)
{
    while (!ast_repr.starts_with('(')) {
        ASSERT_TRUE(ast_repr.size() > 0);
        ast_repr.remove_prefix(1);
    }
    while (!ast_repr.ends_with(')')) {
        ASSERT_TRUE(ast_repr.size() > 0);
        ast_repr.remove_suffix(1);
    }
    assert_expr(std::move(tokens), ast_repr);
}

static void assert_errors(std::vector<Lox::Token> tokens,
                          std::vector<Lox::Error> errors)
{
    Lox::Parser parser(std::move(tokens));
    parser.parse();
    auto& errs = parser.errors();
    ASSERT_EQ(errs.size(), errors.size());
    for (std::size_t i = 0; i < errs.size(); ++i)
        EXPECT_EQ(errs[i].span, errors[i].span);
}

TEST(Parser, EmptyInputReturnsNoTree)
{
    Lox::Parser parser({});
    auto ast = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    EXPECT_FALSE(ast);
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
    assert_sexp({ { TokenType::Minus, "" }, { TokenType::Number, "", 123.0 } }, R"(
(-
  123)
    )");

    assert_errors({ { TokenType::Minus, "" }, { TokenType::Invalid, "foo" } },
        { { "foo", "" } });
}

TEST(Parser, ErrorAtEofPointsAtLastToken)
{
    assert_errors({ { TokenType::Minus, "-" } }, { { "-", "" } });
}

TEST(Parser, MultiplyExpressions)
{
    assert_sexp({
        { TokenType::Number, "", 5.0 },
        { TokenType::Slash, "" },
        { TokenType::Number, "", 7.0 } }, R"(
(/
  5
  7)
    )");
    assert_sexp({
        { TokenType::Number, "", 500.0 },
        { TokenType::Star, "" },
        { TokenType::Number, "", 700.0 } }, R"(
(*
  500
  700)
    )");
    assert_sexp({
        { TokenType::Number, "", 5.0 },
        { TokenType::Slash, "" },
        { TokenType::Number, "", 7.0 },
        { TokenType::Star, "" },
        { TokenType::Number, "", 9.0 } }, R"(
(*
  (/
    5
    7)
  9)
    )");

    assert_errors({
        { TokenType::Number, "", 5.0 },
        { TokenType::Slash, "" },
        { TokenType::Invalid, "foo" },
    }, { { "foo", "" } });
}

TEST(Parser, AddExpressions)
{
    assert_sexp({
        { TokenType::Number, "", 5.0 },
        { TokenType::Plus, "" },
        { TokenType::Number, "", 7.0 } }, R"(
(+
  5
  7)
    )");
    assert_sexp({
        { TokenType::Number, "", 500.0 },
        { TokenType::Minus, "" },
        { TokenType::Number, "", 700.0 } }, R"(
(-
  500
  700)
    )");
    assert_sexp({
        { TokenType::Number, "", 5.0 },
        { TokenType::Plus, "" },
        { TokenType::Number, "", 7.0 },
        { TokenType::Minus, "" },
        { TokenType::Number, "", 9.0 } }, R"(
(-
  (+
    5
    7)
  9)
    )");

    assert_errors({
        { TokenType::Number, "", 5.0 },
        { TokenType::Plus, "" },
        { TokenType::Invalid, "foo" },
    }, { { "foo", "" } });
}
