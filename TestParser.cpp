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

TEST(Parser, ForStatement)
{
    assert_stmt("for c in \"foo\" { c; }", R"(
(for
  c
  "foo"
  (block
    c))
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
(fndecl
  f
  (params)
  (block))
    )");
    assert_stmt("fn f(x) {}", R"(
(fndecl
  f
  (params
    x)
  (block))
    )");
    assert_stmt("fn f(x, y, z) { x = 1; }", R"(
(fndecl
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

    assert_error("fn f _()", "_"); // expected '('
    assert_error("fn f(5)", "5"); // expected identifier
    assert_error("fn f(x, 5)", "5"); // expected identifier
    assert_error("fn f(x {}", "{"); // expected ')'
    assert_error("fn f(x) _", "_"); // expected '{'

    // local vars cannot shadow params
    assert_error("fn f(x) { var x; }", "var x;");
    // but in a block it's fine
    assert_stmt("fn f(x) { { var x; } }", R"(
(fndecl
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
(fndecl
  f
  (params)
  (block
    (return)))
    )");

    assert_stmt("fn f() { return x + y; }", R"(
(fndecl
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

TEST(Parser, FunctionExpression)
{
    // test common case, others are tested by function declaration tests
    assert_expr("fn(x, y) { x = 1; }", R"(
(fn
  (params
    x
    y)
  (block
    (=
      x
      1)))
    )");

    assert_error("fn 5()", "5"); // expected '('
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
