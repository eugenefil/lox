#include "Interpreter.h"
#include <gtest/gtest.h>

static void assert_string(std::shared_ptr<Lox::Expr> ast, std::string_view value)
{
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    ASSERT_TRUE(obj);
    ASSERT_EQ(obj->type_name(), "String");
    ASSERT_EQ(obj->get_string(), value);
}

static void assert_number(std::shared_ptr<Lox::Expr> ast, double value)
{
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    ASSERT_TRUE(obj);
    ASSERT_EQ(obj->type_name(), "Number");
    ASSERT_EQ(obj->get_number(), value);
}

static void assert_bool(std::shared_ptr<Lox::Expr> ast, bool value)
{
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    ASSERT_TRUE(obj);
    ASSERT_EQ(obj->type_name(), "Bool");
    ASSERT_EQ(obj->get_bool(), value);
}

static void assert_nil(std::shared_ptr<Lox::Expr> ast)
{
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    ASSERT_TRUE(obj);
    ASSERT_EQ(obj->type_name(), "NilType");
}

#define EXPR(expr_class, ...) (std::make_shared<Lox::expr_class>(__VA_ARGS__))
#define UNARY(op, expr) EXPR(UnaryExpr, Lox::UnaryOp::op, (expr))

TEST(Interpreter, EvalLiterals)
{
    assert_string(EXPR(StringLiteral, "foo"), "foo");
    assert_number(EXPR(NumberLiteral, 5.0), 5.0);
    assert_bool(EXPR(BoolLiteral, true), true);
    assert_nil(EXPR(NilLiteral));
}

TEST(Interpreter, EvalUnaryExpressions)
{
    assert_number(UNARY(Minus, EXPR(NumberLiteral, 5.0)), -5.0);
    assert_bool(UNARY(Not, EXPR(StringLiteral, "foo")), false);
    assert_bool(UNARY(Not, EXPR(StringLiteral, "")), true);
    assert_bool(UNARY(Not, EXPR(NumberLiteral, 5.0)), false);
    assert_bool(UNARY(Not, EXPR(NumberLiteral, 0.0)), true);
    assert_bool(UNARY(Not, EXPR(BoolLiteral, true)), false);
    assert_bool(UNARY(Not, EXPR(BoolLiteral, false)), true);
    assert_bool(UNARY(Not, EXPR(NilLiteral)), true);
}
