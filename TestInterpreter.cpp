#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <gtest/gtest.h>

static void assert_value(std::string_view input,
                         std::shared_ptr<Lox::Object> value)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    ASSERT_FALSE(parser.has_errors());
    ASSERT_TRUE(ast);
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    ASSERT_TRUE(obj);
    ASSERT_TRUE(value);
    ASSERT_TRUE(obj->__eq__(*value));
}

static void assert_string(std::string_view input, std::string_view value)
{
    assert_value(input, std::make_shared<Lox::String>(value));
}

static void assert_number(std::string_view input, double value)
{
    assert_value(input, std::make_shared<Lox::Number>(value));
}

static void assert_bool(std::string_view input, bool value)
{
    assert_value(input, std::make_shared<Lox::Bool>(value));
}

static void assert_nil(std::string_view input)
{
    assert_value(input, std::make_shared<Lox::NilType>());
}

static void assert_error(std::string_view input)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());
    Lox::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    ASSERT_FALSE(parser.has_errors());
    ASSERT_TRUE(ast);
    Lox::Interpreter interp(ast);
    auto obj = interp.interpret();
    EXPECT_FALSE(obj);
    auto& errs = interp.errors();
    ASSERT_EQ(errs.size(), 1);
    ASSERT_EQ(errs[0].span, input);
}

TEST(Interpreter, EmptyAst)
{
    Lox::Interpreter interp { std::shared_ptr<Lox::Expr>() };
    auto obj = interp.interpret();
    EXPECT_FALSE(interp.has_errors());
    EXPECT_FALSE(obj);
}

TEST(Interpreter, EvalLiterals)
{
    assert_string(R"("foo")", "foo");
    assert_number("5", 5.0);
    assert_bool("true", true);
    assert_bool("false", false);
    assert_nil("nil");
}

TEST(Interpreter, EvalUnaryExpressions)
{
    assert_number("-5", -5.0);
    assert_error(R"(-"foo")");

    assert_bool(R"(!"foo")", false);
    assert_bool(R"(!"")", true);
    assert_bool("!5", false);
    assert_bool("!0", true);
    assert_bool("!true", false);
    assert_bool("!false", true);
    assert_bool("!nil", true);
}

TEST(Interpreter, EvalBinaryExpressions)
{
    assert_number("10 / 2", 5.0);
    assert_error("10 / nil");

    assert_number("10 * 2", 20.0);
    assert_error("10 * nil");

    assert_number("10 + 2", 12.0);
    assert_string(R"("foo" + "bar")", "foobar");
    assert_error("10 + nil");

    assert_number("2 - 10", -8.0);
    assert_error("2 - nil");

    assert_bool("5 == 5", true);
    assert_bool("5 == 7", false);
    assert_bool("5 == nil", false);
    assert_bool(R"("foo" == "foo")", true);
    assert_bool(R"("foo" == "bar")", false);
    assert_bool(R"("foo" == nil)", false);
    assert_bool("true == true", true);
    assert_bool("true == false", false);
    assert_bool("true == nil", false);
    assert_bool("nil == nil", true);
    assert_bool("nil == 5", false);

    assert_bool("5 < 7", true);
    assert_bool("5 < 5", false);
    assert_error("5 < nil");
    assert_bool(R"("aaa" < "bbb")", true);
    assert_bool(R"("aaa" < "aaa")", false);

    assert_bool("5 <= 7", true);
    assert_bool("5 <= 5", true);
    assert_bool("5 <= 4", false);
    assert_error("5 <= nil");
    assert_bool(R"("aaa" <= "bbb")", true);
    assert_bool(R"("aaa" <= "aaa")", true);
    assert_bool(R"("aaa" <= "000")", false);

    assert_bool("5 > 4", true);
    assert_bool("5 > 5", false);
    assert_error("5 > nil");
    assert_bool(R"("bbb" > "aaa")", true);
    assert_bool(R"("bbb" > "bbb")", false);

    assert_bool("5 >= 4", true);
    assert_bool("5 >= 5", true);
    assert_bool("5 >= 7", false);
    assert_error("5 >= nil");
    assert_bool(R"("bbb" >= "aaa")", true);
    assert_bool(R"("bbb" >= "bbb")", true);
    assert_bool(R"("bbb" >= "ccc")", false);
}
