#include "Parser.h"
#include <gtest/gtest.h>

using Lox::TokenType;

static void assert_program(std::string_view source, std::string_view sexp)
{
    Lox::Lexer lexer(source);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens), source);
    auto program = parser.parse();
    EXPECT_FALSE(parser.has_errors());
    ASSERT_TRUE(program);
    EXPECT_EQ(program->text(), source);

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

static void assert_expr(std::string_view source, std::string_view sexp)
{
    assert_program(std::string(source) + ';', strip_sexp(sexp));
}

static void assert_literal(std::string_view source, std::string_view sexp)
{
    assert_program(std::string(source) + ';', sexp);
}

static void assert_stmt(std::string_view source, std::string_view sexp)
{
    assert_program(source, strip_sexp(sexp));
}

static void assert_error(std::string_view source, std::string_view error_span)
{
    Lox::Lexer lexer(source);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens), source);
    auto program = parser.parse();
    EXPECT_FALSE(program);
    auto& errs = parser.errors();
    ASSERT_EQ(errs.size(), 1);
    EXPECT_EQ(errs[0].source, source);
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
    assert_expr("5 / 7 * 9 % 2", R"(
(%
  (*
    (/
      5
      7)
    9)
  2)
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

TEST(Parser, LogicExpressions)
{
    // comparison has higher precedence
    assert_expr("5 == 5 and 3 != 3", R"(
(and
  (==
    5
    5)
  (!=
    3
    3))
    )");
    // left associativity
    assert_expr("0 and 1 and 2", R"(
(and
  (and
    0
    1)
  2)
    )");
    assert_error("true and /", "/");

    // logic 'and' has higher precedence
    assert_expr("1 and 3 or 5 and 7", R"(
(or
  (and
    1
    3)
  (and
    5
    7))
    )");
    // left associativity
    assert_expr("0 or 1 or 2", R"(
(or
  (or
    0
    1)
  2)
    )");
    assert_error("0 or /", "/");
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

TEST(Parser, ForStatement)
{
    assert_stmt("for c in \"foo\" { print c; }", R"(
(for
  c
  "foo"
  (block
    (print
      c)))
    )");

    assert_error("for 5 in", "5"); // expected identifier
    assert_error("for c _", "_"); // expected 'in'
    assert_error("for c in /", "/"); // expected expression
    assert_error("for c in \"foo\" _", "_"); // expected '{'
}

TEST(Parser, BreakStatement)
{
    // check that parser sees break as inside the loop
    assert_stmt("while true { break; }", R"(
(while
  true
  (block
    (break)))
    )");

    // same for 'for' loop
    assert_stmt("for c in \"foo\" { break; }", R"(
(for
  c
  "foo"
  (block
    (break)))
    )");

    assert_error("break;", "break"); // break outside loop
    assert_error("while true {} break;", "break");
    assert_error("while true { break }", "}"); // expected ';'
}

TEST(Parser, ContinueStatement)
{
    // check that parser sees continue as inside the loop
    assert_stmt("while true { continue; }", R"(
(while
  true
  (block
    (continue)))
    )");

    // same for 'for' loop
    assert_stmt("for c in \"foo\" { continue; }", R"(
(for
  c
  "foo"
  (block
    (continue)))
    )");

    assert_error("continue;", "continue"); // continue outside loop
    assert_error("while true {} continue;", "continue");
    assert_error("while true { continue }", "}"); // expected ';'
}

TEST(Parser, FunctionDeclaration)
{
    assert_stmt("fn f() {}", R"(
(fn
  f
  (params)
  (block))
    )");
    assert_stmt("fn f(x) {}", R"(
(fn
  f
  (params
    x)
  (block))
    )");
    assert_stmt("fn f(x, y, z) { x = 1; }", R"(
(fn
  f
  (params
    x
    y
    z)
  (block
    (=
      x
      1)))
    )");

    assert_error("fn 5()", "5"); // identifier expected
    assert_error("fn f _()", "_"); // expected '('
    assert_error("fn f(5)", "5"); // identifier expected
    assert_error("fn f(x, 5)", "5"); // identifier expected
    assert_error("fn f(x {}", "{"); // expected ')'
    assert_error("fn f(x) _", "_"); // expected '{'

    // local vars cannot shadow params
    assert_error("fn f(x) { var x; }", "var x;");
    // but in a block it's fine
    assert_stmt("fn f(x) { { var x; } }", R"(
(fn
  f
  (params
    x)
  (block
    (block
      (var
        x))))
    )");
}

TEST(Parser, CallExpression)
{
    assert_expr("f()", R"(
(call
  f
  (args))
    )");
    assert_expr("f(5)", R"(
(call
  f
  (args
    5))
    )");
    assert_expr("f(5, 7, 9)", R"(
(call
  f
  (args
    5
    7
    9))
    )");
    assert_expr("f(5, 7, 9)(\"foo\")(true)", R"(
(call
  (call
    (call
      f
      (args
        5
        7
        9))
    (args
      "foo"))
  (args
    true))
    )");

    assert_expr("(f)()", R"(
(call
  (group
    f)
  (args))
    )");

    // unary binds lower than call
    assert_expr("-f()", R"(
(-
  (call
    f
    (args)))
    )");

    assert_error("f(/)", "/"); // expected expression
    assert_error("f(5,)", ")"); // expected expression
    assert_error("f(5 _", "_"); // expected ')'
}

TEST(Parser, ReturnStatement)
{
    assert_stmt("fn f() { return; }", R"(
(fn
  f
  (params)
  (block
    (return)))
    )");

    assert_stmt("fn f() { return x + y; }", R"(
(fn
  f
  (params)
  (block
    (return
      (+
        x
        y))))
    )");

    assert_error("return;", "return"); // return outside function
    assert_error("fn f() {} return;", "return"); // same
    assert_error("fn f() { return /; }", "/"); // expected expression
    assert_error("fn f() { return 5 _ }", "_"); // expected ';'
}
