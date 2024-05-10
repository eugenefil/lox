#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <gtest/gtest.h>

static void assert_env_multi_program(std::vector<std::string_view> inputs,
                                     const Lox::Interpreter::EnvType& env,
                                     std::vector<std::string_view> error_spans = {})
{
    if (error_spans.size() > 0) {
        ASSERT_EQ(error_spans.size(), inputs.size());
    }

    Lox::Interpreter interp;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        Lox::Lexer lexer(inputs[i]);
        auto tokens = lexer.lex();
        ASSERT_FALSE(lexer.has_errors());

        Lox::Parser parser(std::move(tokens));
        auto program = parser.parse();
        ASSERT_FALSE(parser.has_errors());
        ASSERT_TRUE(program);

        interp.interpret(program);
        if (error_spans.empty() || error_spans[i].empty())
            ASSERT_FALSE(interp.has_errors());
        else {
            auto& errs = interp.errors();
            ASSERT_EQ(errs.size(), 1);
            ASSERT_EQ(errs[0].span, error_spans[i]);
        }
    }

    auto& e = interp.env();
    EXPECT_EQ(e.size(), env.size());
    for (auto& [name, value] : env) {
        ASSERT_TRUE(e.contains(name));
        auto& obj = e.at(name);
        ASSERT_TRUE(obj);
        ASSERT_TRUE(value);
        EXPECT_TRUE(obj->__eq__(*value));
    }
}

static void assert_env(std::string_view input,
                       const Lox::Interpreter::EnvType& env)
{
    assert_env_multi_program({ input }, env);
}

static void assert_env_and_error(std::string_view input,
                                 const Lox::Interpreter::EnvType& env,
                                 std::string_view error_span)
{
    assert_env_multi_program({ input }, env, { error_span });
}

static void assert_value(std::string_view input,
                         std::shared_ptr<Lox::Object> value)
{
    assert_env(std::string("var x = ").append(input) + ';',
               { { "x", value } });
}

static void assert_string(std::string_view input, std::string_view value)
{
    assert_value(input, Lox::make_string(value));
}

static void assert_number(std::string_view input, double value)
{
    assert_value(input, Lox::make_number(value));
}

static void assert_bool(std::string_view input, bool value)
{
    assert_value(input, Lox::make_bool(value));
}

static void assert_nil(std::string_view input)
{
    assert_value(input, Lox::make_nil());
}

static void assert_error(std::string_view input, std::string_view error_span)
{
    Lox::Lexer lexer(input);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());

    Lox::Parser parser(std::move(tokens));
    auto program = parser.parse();
    ASSERT_FALSE(parser.has_errors());
    ASSERT_TRUE(program);

    Lox::Interpreter interp;
    interp.interpret(program);
    auto& errs = interp.errors();
    ASSERT_EQ(errs.size(), 1);
    ASSERT_EQ(errs[0].span, error_span);
}

static void assert_value_error(std::string_view input)
{
    assert_error(std::string("var x = ").append(input) + ';', input);
}

TEST(Interpreter, EmptyProgram)
{
    Lox::Interpreter interp;
    interp.interpret(std::make_shared<Lox::Program>(
        std::vector<std::shared_ptr<Lox::Stmt>>(), ""));
    EXPECT_FALSE(interp.has_errors());
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
    assert_value_error(R"(-"foo")");

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
    assert_value_error("10 / nil");

    assert_number("10 * 2", 20.0);
    assert_value_error("10 * nil");

    assert_number("10 + 2", 12.0);
    assert_string(R"("foo" + "bar")", "foobar");
    assert_value_error("10 + nil");

    assert_number("2 - 10", -8.0);
    assert_value_error("2 - nil");

    assert_bool("5 == 5", true);
    assert_bool("5 == 7", false);
    assert_value_error("5 == nil");
    assert_bool(R"("foo" == "foo")", true);
    assert_bool(R"("foo" == "bar")", false);
    assert_bool("true == true", true);
    assert_bool("true == false", false);
    assert_bool("nil == nil", true);

    assert_bool("5 < 7", true);
    assert_bool("5 < 5", false);
    assert_value_error("5 < nil");
    assert_bool(R"("aaa" < "bbb")", true);
    assert_bool(R"("aaa" < "aaa")", false);

    assert_bool("5 <= 7", true);
    assert_bool("5 <= 5", true);
    assert_bool("5 <= 4", false);
    assert_value_error("5 <= nil");
    assert_bool(R"("aaa" <= "bbb")", true);
    assert_bool(R"("aaa" <= "aaa")", true);
    assert_bool(R"("aaa" <= "000")", false);

    assert_bool("5 > 4", true);
    assert_bool("5 > 5", false);
    assert_value_error("5 > nil");
    assert_bool(R"("bbb" > "aaa")", true);
    assert_bool(R"("bbb" > "bbb")", false);

    assert_bool("5 >= 4", true);
    assert_bool("5 >= 5", true);
    assert_bool("5 >= 7", false);
    assert_value_error("5 >= nil");
    assert_bool(R"("bbb" >= "aaa")", true);
    assert_bool(R"("bbb" >= "bbb")", true);
    assert_bool(R"("bbb" >= "ccc")", false);
}

TEST(Interpreter, EvalIdentifier)
{
    assert_env("var x = 5; var y = x * 2;", {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(10) },
    });

    assert_error("var x = y;", "y");
}

TEST(Interpreter, ExecuteVarStatements)
{
    assert_env("var x;", { { "x", Lox::make_nil() } });

    assert_env("var x = 5; var s = \"foo\"; var b = true;", {
        { "x", Lox::make_number(5) },
        { "s", Lox::make_string("foo") },
        { "b", Lox::make_bool(true) },
    });

    assert_env("var x = 5; var x = \"foo\";", { { "x", Lox::make_string("foo") } });
}

TEST(Interpreter, ExecutePrintStatements)
{
    assert_error("print -nil;", "-nil");
}

TEST(Interpreter, ProgramsShareEnv)
{
    assert_env_multi_program({ "var x = 5;", "var y = x * 2;" }, {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(10) },
    });
}

TEST(Interpreter, ExecuteAssignStatements)
{
    assert_env("var x = 5; x = x + 7;", { { "x", Lox::make_number(12) } });

    assert_error("x = foo;", "foo");
    assert_error("x = 5;", "x");
}

TEST(Interpreter, ExecuteBlockStatements)
{
    assert_env("var x = 5; { var y = 7; x = x + y; }", {
        { "x", Lox::make_number(12) },
    });
    assert_env("var x = 5; var y = 7; { var x = x + 10; y = x; }", {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(15) },
    });
}

TEST(Interpreter, EnvIsRestoredAfterError)
{
    assert_env_multi_program({ "var x; { var y; foo; }" },
        { { "x", Lox::make_nil() } },
        { "foo" });
}

TEST(Interpreter, ExecuteIfStatements)
{
    assert_env("var x = 5; if x > 0 { x = 7; }", {
        { "x", Lox::make_number(7) },
    });
    assert_env("var x = -1; if x > 0 { x = 7; }", {
        { "x", Lox::make_number(-1) },
    });
    assert_env("var x = 5; if x > 0 { x = 7; } else { x = 3; }", {
        { "x", Lox::make_number(7) },
    });
    assert_env("var x = -1; if x > 0 { x = 7; } else { x = 3; }", {
        { "x", Lox::make_number(3) },
    });
    assert_env("var x = -1; if x > 0 { x = 7; } else if x < 0 { x = 3; }", {
        { "x", Lox::make_number(3) },
    });
    assert_env("var x = 0; if x > 0 { x = 7; } else if x < 0 { x = 3; }", {
        { "x", Lox::make_number(0) },
    });

    assert_error("if x { y; }", "x");
    assert_error("if true { x; } else { y; }", "x");
    assert_error("if false { x; } else { y; }", "y");
}

TEST(Interpreter, ExecuteWhileStatements)
{
    assert_env("var x = 5; while false { x = 7; }", {
        { "x", Lox::make_number(5) },
    });
    assert_env("var x = 3; var y = 0; while x > 0 { x = x - 1; y = y + 1; }", {
        { "x", Lox::make_number(0) },
        { "y", Lox::make_number(3) },
    });

    assert_error("while x { y; }", "x");
    assert_error("while true { y; }", "y");
    // test loop is executed before error happens
    assert_env_and_error("var x = 0; while true { x = x + 1; if x == 3 { y; } }",
        { { "x", Lox::make_number(3) } },
        "y"
    );
    // test loop finishes normally if error is not triggered
    assert_env("var x = 3; while x > 0 { x = x - 1; if x == 5 { y; } }", {
        { "x", Lox::make_number(0) },
    });
}
