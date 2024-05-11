#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

static void assert_program(std::string_view input, std::string_view sexp)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto program = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    ASSERT_TRUE(program);
    EXPECT_EQ(program->text(), input);

    auto indented_sexp = std::string(sexp);
    for (std::size_t i = indented_sexp.find('\n'); i != indented_sexp.npos;) {
        indented_sexp.insert(i + 1, "  ");
        i = indented_sexp.find('\n', i + 1);
    }
    if (!indented_sexp.empty())
        indented_sexp.insert(0, "\n  "); // add indent for line 1 if any
    EXPECT_EQ(program->dump(0), "(program" + indented_sexp + ')');
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
    auto program = parser.parse();
    EXPECT_FALSE(program);
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

TEST(Parser, AssignStatement)
{
    assert_stmt("x = 5 + 7;", R"(
(=
  x
  (+
    5
    7))
    )");

    assert_error("x = /", "/");
    assert_error("x = 5_", "_");
}

TEST(Parser, BlockStatement)
{
    assert_stmt("{}", "(block)");
    assert_stmt("{ var x = 5; { var x = 7; } }", R"(
(block
  (var
    x
    5)
  (block
    (var
      x
      7)))
    )");

    assert_error("{ foo;", "{");
}

TEST(Parser, IfStatement)
{
    assert_stmt("if x > 0 { print x; }", R"(
(if
  (>
    x
    0)
  (block
    (print
      x)))
    )");

    assert_stmt("if x > 0 { print x; } else { print y; }", R"(
(if
  (>
    x
    0)
  (block
    (print
      x))
  (block
    (print
      y)))
    )");

    assert_stmt("if x > 0 { print x; } else if y > 0 { print y; }", R"(
(if
  (>
    x
    0)
  (block
    (print
      x))
  (if
    (>
      y
      0)
    (block
      (print
        y))))
    )");

    assert_error("if / ", "/");
    assert_error("if x _", "_");
    assert_error("if x {} else _", "_");
}

TEST(Parser, WhileStatement)
{
    assert_stmt("while x > 0 { x = x - 1; }", R"(
(while
  (>
    x
    0)
  (block
    (=
      x
      (-
        x
        1))))
    )");

    assert_error("while / {}", "/");
    assert_error("while 1 {_", "_");
}

TEST(Parser, BreakStatement)
{
    assert_stmt("while true { break; }", R"(
(while
  true
  (block
    (break)))
    )");

    assert_error("break;", "break"); // break outside loop
    assert_error("while true {} break;", "break");
    assert_error("while true { break }", "}"); // expected ';'
}
