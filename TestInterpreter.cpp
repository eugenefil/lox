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

TEST(Interpreter, Literals)
{
    assert_string(std::make_shared<Lox::StringLiteral>("foo"), "foo");
    assert_number(std::make_shared<Lox::NumberLiteral>(5.0), 5.0);
    assert_bool(std::make_shared<Lox::BoolLiteral>(true), true);
    assert_nil(std::make_shared<Lox::NilLiteral>());
}
