#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

static void assert_program(std::string_view input, std::string_view sexp)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    ASSERT_TRUE(ast);
    EXPECT_EQ(ast->text(), input);

    auto indented_sexp = std::string(sexp);
    for (std::size_t i = indented_sexp.find('\n'); i != indented_sexp.npos;) {
        indented_sexp.insert(i + 1, "  ");
        i = indented_sexp.find('\n', i + 1);
    }
    if (!indented_sexp.empty())
        indented_sexp.insert(0, "\n  "); // add indent for line 1 if any
    EXPECT_EQ(ast->dump(0), "(program" + indented_sexp + ')');
}

static std::string_view strip_sexp(std::string_view sexp)
{
    while (!sexp.starts_with('(')) {
        assert(sexp.size() > 0);
        sexp.remove_prefix(1);
    }
    while (!sexp.ends_with(')')) {
        assert(sexp.size() > 0);
        sexp.remove_suffix(1);
    }
    return sexp;
}

static void assert_expr(std::string_view input, std::string_view sexp)
{
    assert_program(std::string(input) + ';', strip_sexp(sexp));
}

static void assert_literal(std::string_view input, std::string_view sexp)
{
    assert_program(std::string(input) + ';', sexp);
}

static void assert_stmt(std::string_view input, std::string_view sexp)
{
    assert_program(input, strip_sexp(sexp));
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

TEST(Parser, EmptyProgram)
{
    assert_program("", "");
}

TEST(Parser, PrimaryExpressions)
{
    assert_literal(R"("")", R"("")");
    assert_literal(R"("Hello world!")", R"("Hello world!")");
    assert_literal(R"("\t\r\n\"\\")", R"("\t\r\n\"\\")");

    assert_literal("123", "123");
    assert_literal("3.14159265", "3.14159265");

    assert_literal("foo", "foo");

    assert_literal("true", "true");
    assert_literal("false", "false");

    assert_literal("nil", "nil");

    assert_error("/", "/");
}

TEST(Parser, UnaryExpressions)
{
    assert_expr("-123", R"(
(-
  123)
    )");
    assert_expr("!true", R"(
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
    assert_expr("5 / 7", R"(
(/
  5
  7)
    )");
    assert_expr("500 * 700", R"(
(*
  500
  700)
    )");
    assert_expr("5 / 7 * 9", R"(
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
    assert_expr("5 + 7", R"(
(+
  5
  7)
    )");
    assert_expr("500 - 700", R"(
(-
  500
  700)
    )");
    assert_expr("5 + 7 - 9", R"(
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
    assert_expr("5 == 7", R"(
(==
  5
  7)
    )");
    assert_expr("5 != 7", R"(
(!=
  5
  7)
    )");
    assert_expr("5 < 7", R"(
(<
  5
  7)
    )");
    assert_expr("5 <= 7", R"(
(<=
  5
  7)
    )");
    assert_expr("5 > 7", R"(
(>
  5
  7)
    )");
    assert_expr("5 >= 7", R"(
(>=
  5
  7)
    )");

    assert_error("5 == /", "/");
}

TEST(Parser, GroupExpression)
{
    assert_expr("(5 + 7) * 9", R"(
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

TEST(Parser, ExpressionStatement)
{
    assert_error("5 + 7_", "_");
    assert_stmt("5 + 7; 5 * 8;", R"(
(+
  5
  7)
(*
  5
  8)
    )");
}

TEST(Parser, VarStatement)
{
    assert_stmt("var x;", R"(
(var
  x)
    )");
    assert_stmt("var x = 5 + 7;", R"(
(var
  x
  (+
    5
    7))
    )");

    assert_error("var 5;", "5");
    assert_error("var x = ;", ";");
    assert_error("var x = 5_", "_");
}

TEST(Parser, PrintStatement)
{
    assert_stmt("print;", "(print)");
    assert_stmt("print 5 + 7;", R"(
(print
  (+
    5
    7))
    )");

    assert_error("print /", "/");
    assert_error("print 5_", "_");
}
