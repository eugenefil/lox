#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

static void assert_expr(std::string_view input, std::string_view ast_repr)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    ASSERT_TRUE(ast);
    EXPECT_EQ(ast->dump(0), ast_repr);
}

static void assert_sexp(std::string_view input, std::string_view ast_repr)
{
    while (!ast_repr.starts_with('(')) {
        ASSERT_TRUE(ast_repr.size() > 0);
        ast_repr.remove_prefix(1);
    }
    while (!ast_repr.ends_with(')')) {
        ASSERT_TRUE(ast_repr.size() > 0);
        ast_repr.remove_suffix(1);
    }
    assert_expr(input, ast_repr);
}

static void assert_error(std::string_view input, std::string_view error_span)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    EXPECT_FALSE(ast);
    auto& errs = parser.errors();
    ASSERT_EQ(errs.size(), 1);
    EXPECT_EQ(errs[0].span, error_span);
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
    assert_expr(R"("")", R"("")");
    assert_expr(R"("Hello world!")", R"("Hello world!")");
    assert_expr(R"("\t\r\n\"\\")", R"("\t\r\n\"\\")");

    assert_expr("123", "123");
    assert_expr("3.14159265", "3.14159265");

    assert_expr("foo", "foo");

    assert_expr("true", "true");
    assert_expr("false", "false");

    assert_expr("nil", "nil");

    assert_error("/", "/");
}

TEST(Parser, UnaryExpressions)
{
    assert_sexp("-123", R"(
(-
  123)
    )");
    assert_sexp("!true", R"(
(!
  true)
    )");

    assert_error("-/", "/");
}

TEST(Parser, ErrorAtEofPointsAtLastToken)
{
    assert_error("-", "-");
}

TEST(Parser, MultiplyExpressions)
{
    assert_sexp("5 / 7", R"(
(/
  5
  7)
    )");
    assert_sexp("500 * 700", R"(
(*
  500
  700)
    )");
    assert_sexp("5 / 7 * 9", R"(
(*
  (/
    5
    7)
  9)
    )");

    assert_error("5 * /", "/");
}

TEST(Parser, AddExpressions)
{
    assert_sexp("5 + 7", R"(
(+
  5
  7)
    )");
    assert_sexp("500 - 700", R"(
(-
  500
  700)
    )");
    assert_sexp("5 + 7 - 9", R"(
(-
  (+
    5
    7)
  9)
    )");

    assert_error("5 + /", "/");
}

TEST(Parser, CompareExpressions)
{
    assert_sexp("5 == 7", R"(
(==
  5
  7)
    )");
    assert_sexp("5 != 7", R"(
(!=
  5
  7)
    )");
    assert_sexp("5 < 7", R"(
(<
  5
  7)
    )");
    assert_sexp("5 <= 7", R"(
(<=
  5
  7)
    )");
    assert_sexp("5 > 7", R"(
(>
  5
  7)
    )");
    assert_sexp("5 >= 7", R"(
(>=
  5
  7)
    )");

    assert_error("5 == /", "/");
}

TEST(Parser, GroupExpression)
{
    assert_sexp("(5 + 7) * 9", R"(
(*
  (group
    (+
      5
      7))
  9)
    )");

    assert_error("(/", "/");
    assert_error("(5 + 7", "(");
}
