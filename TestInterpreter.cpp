#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <gtest/gtest.h>

class DummyFunction : public Lox::Object {
public:
    explicit DummyFunction(std::string_view dump) : m_dump(dump)
    {
        while (!m_dump.starts_with('(')) {
            assert(!m_dump.empty());
            m_dump.remove_prefix(1);
        }
    }

    std::string_view type_name() const override { return "DummyFunction"; }
    std::string dump() const { return std::string(m_dump); }

private:
    std::string_view m_dump;
};

static void assert_env_multi_program(std::vector<std::string_view> sources,
                                     const Lox::Interpreter::EnvType& env,
                                     std::vector<std::string_view> error_spans = {})
{
    if (error_spans.size() > 0) {
        ASSERT_EQ(error_spans.size(), sources.size());
    }

    Lox::Interpreter interp;
    for (std::size_t i = 0; i < sources.size(); ++i) {
        Lox::Lexer lexer(sources[i]);
        auto tokens = lexer.lex();
        ASSERT_FALSE(lexer.has_errors());

        Lox::Parser parser(std::move(tokens), sources[i]);
        auto program = parser.parse();
        ASSERT_FALSE(parser.has_errors());
        ASSERT_TRUE(program);

        interp.interpret(program);
        if (error_spans.empty() || error_spans[i].empty())
            ASSERT_FALSE(interp.has_errors());
        else {
            auto& errs = interp.errors();
            ASSERT_EQ(errs.size(), 1);
            ASSERT_EQ(errs[0].source, sources[i]);
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
        if (value->type_name() == "DummyFunction") {
            ASSERT_TRUE(obj->is_function());
            auto& dummy = static_cast<DummyFunction&>(*value);
            auto& func = static_cast<Lox::Function&>(*obj);
            EXPECT_EQ(func.decl()->dump(0), dummy.dump());
        } else
            EXPECT_TRUE(value->__eq__(*obj));
    }
}

static void assert_env(std::string_view source,
                       const Lox::Interpreter::EnvType& env)
{
    assert_env_multi_program({ source }, env);
}

static void assert_env_and_error(std::string_view source,
                                 const Lox::Interpreter::EnvType& env,
                                 std::string_view error_span)
{
    assert_env_multi_program({ source }, env, { error_span });
}

static void assert_value(std::string_view source,
                         std::shared_ptr<Lox::Object> value)
{
    assert_env(std::string("var x = ").append(source) + ';',
               { { "x", value } });
}

static void assert_string(std::string_view source, std::string_view value)
{
    assert_value(source, Lox::make_string(value));
}

static void assert_number(std::string_view source, double value)
{
    assert_value(source, Lox::make_number(value));
}

static void assert_bool(std::string_view source, bool value)
{
    assert_value(source, Lox::make_bool(value));
}

static void assert_nil(std::string_view source)
{
    assert_value(source, Lox::make_nil());
}

static void assert_error(std::string_view source, std::string_view error_span)
{
    Lox::Lexer lexer(source);
    auto tokens = lexer.lex();
    ASSERT_FALSE(lexer.has_errors());

    Lox::Parser parser(std::move(tokens), source);
    auto program = parser.parse();
    ASSERT_FALSE(parser.has_errors());
    ASSERT_TRUE(program);

    Lox::Interpreter interp;
    interp.interpret(program);
    auto& errs = interp.errors();
    ASSERT_EQ(errs.size(), 1);
    ASSERT_EQ(errs[0].source, source);
    ASSERT_EQ(errs[0].span, error_span);
}

static void assert_value_error(std::string_view source,
                               std::string_view error_span = {})
{
    if (error_span.empty())
        error_span = source;
    assert_error(std::string("var x = ").append(source) + ';', error_span);
}

TEST(Interpreter, EmptyProgram)
{
    Lox::Interpreter interp;
    interp.interpret(std::make_shared<Lox::Program>(
        std::vector<std::shared_ptr<Lox::Stmt>>(), ""));
    EXPECT_FALSE(interp.has_errors());
}

TEST(Interpreter, Literals)
{
    assert_string(R"("foo")", "foo");
    assert_number("5", 5.0);
    assert_bool("true", true);
    assert_bool("false", false);
    assert_nil("nil");
}

TEST(Interpreter, UnaryExpression)
{
    assert_number("-5", -5.0);
    assert_number("--5", 5.0);
    assert_value_error(R"(-"foo")");

    assert_bool("!true", false);
    assert_bool("!false", true);
    assert_bool("!!false", false);
    assert_value_error("!nil");
    assert_value_error("!0");
    assert_value_error(R"(!"")");
}

TEST(Interpreter, BinaryExpression)
{
    assert_number("10 / 2", 5.0);
    assert_value_error("10 / nil");

    assert_number("10 * 2", 20.0);
    assert_value_error("10 * nil");

    assert_number("10 % 3", 1.0);
    assert_value_error("10 % nil");

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

    assert_bool("false and false", false);
    assert_bool("false and true", false);
    assert_bool("true and false", false);
    assert_bool("true and true", true);
    assert_value_error("1 and 2", "1"); // not boolean
    assert_value_error("true and 2", "2"); // not boolean
    assert_bool("false and 2", false); // short-circuit

    assert_bool("false or false", false);
    assert_bool("false or true", true);
    assert_bool("true or false", true);
    assert_bool("true or true", true);
    assert_value_error("1 or 2", "1"); // not boolean
    assert_value_error("false or 2", "2"); // not boolean
    assert_bool("true or 2", true); // short-circuit
}

TEST(Interpreter, Identifier)
{
    assert_env("var x = 5; var y = x * 2;", {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(10) },
    });

    assert_error("var x = y;", "y");
}

TEST(Interpreter, VarStatement)
{
    assert_env("var x;", { { "x", Lox::make_nil() } });

    assert_env("var x = 5; var s = \"foo\"; var b = true;", {
        { "x", Lox::make_number(5) },
        { "s", Lox::make_string("foo") },
        { "b", Lox::make_bool(true) },
    });

    assert_env("var x = 5; var x = \"foo\";", { { "x", Lox::make_string("foo") } });
}

TEST(Interpreter, PrintStatement)
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

TEST(Interpreter, AssignStatement)
{
    assert_env("var x = 5; x = x + 7;", { { "x", Lox::make_number(12) } });

    assert_error("x = foo;", "foo");
    assert_error("x = 5;", "x");
}

TEST(Interpreter, BlockStatement)
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

TEST(Interpreter, IfStatement)
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

    assert_error("if x { y; }", "x"); // test eval error
    assert_error("if 1 { y; }", "1"); // expected boolean
    assert_error("if true { x; } else { y; }", "x"); // 'then' block fails
    assert_error("if false { x; } else { y; }", "y"); // 'else' block fails
}

TEST(Interpreter, WhileStatement)
{
    assert_env("var x = 5; while false { x = 7; }", {
        { "x", Lox::make_number(5) },
    });
    assert_env("var x = 3; var y = 0; while x > 0 { x = x - 1; y = y + 1; }", {
        { "x", Lox::make_number(0) },
        { "y", Lox::make_number(3) },
    });

    assert_error("while x { y; }", "x"); // test eval error
    assert_error("while 1 { y; }", "1"); // expected boolean
    assert_error("while true { y; }", "y"); // block fails
    // loop is executed before error happens
    assert_env_and_error("var x = 0; while true { x = x + 1; if x == 3 { y; } }",
        { { "x", Lox::make_number(3) } },
        "y"
    );
    // loop finishes normally if error is not triggered
    assert_env("var x = 3; while x > 0 { x = x - 1; if x == 5 { y; } }", {
        { "x", Lox::make_number(0) },
    });
}

TEST(Interpreter, ForStatement)
{
    // no execution when collection is empty
    assert_env("var s = \"foo\"; for c in \"\" { s = \"bar\"; }", {
        { "s", Lox::make_string("foo") },
    });

    // loop executes as many times as elements in collection
    assert_env(R"(
        var s = "foo";
        var x = 0;
        for c in "bar" {
            x = x + 1;
            s = s + c;
        })", {
        { "s", Lox::make_string("foobar") },
        { "x", Lox::make_number(3) },
    });

    // loop var is local to the loop
    assert_error("for c in \"bar\" {} c;", "c");

    // loop var shadows same var in outer scope
    assert_env("var c = \"foo\"; for c in \"bar\" { c = \"baz\"; }", {
        { "c", Lox::make_string("foo") },
    });

    // 2 loops iterating on the same collection at the same time
    assert_env(R"(
        var s = "ab";
        var r = "";
        for c in s {
            r = r + c + ":";
            for c in s { r = r + c; }
            r = r + "\n";
        })", {
        { "s", Lox::make_string("ab") },
        { "r", Lox::make_string("a:ab\nb:ab\n") },
    });

    assert_error("for c in x {}", "x"); // expression eval fails
    assert_error("for c in 5 {}", "5"); // expression value is not iterable
    // TODO expression is iterable, but iterator request fails
    // TODO expression is iterable, but fails on some iteration
    assert_error("for c in \"foo\" { x; }", "x"); // block execution fails
}

TEST(Interpreter, BreakStatement)
{
    // check that break:
    // - does not let the rest of the loop body execute
    // - makes current iteration last, i.e. does not function like continue
    // - can be nested into other statements
    // - only affects inner loop
    assert_env(R"(
        var x = 5;
        var y = 0;
        while y < 3 {
            while true {
                if true { break; }
                x = 100;
            }
            y = y + 1;
        })", {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(3) },
    });

    assert_env(R"(
        var x = 5;
        var y = 0;
        var s = "";
        for c in "foo" {
            for c in "bar" {
                s = c;
                if true { break; }
                x = 100;
            }
            y = y + 1;
        })", {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(3) },
        { "s", Lox::make_string("b") },
    });
}

TEST(Interpreter, ContinueStatement)
{
    // check that continue:
    // - does not let the rest of the loop body execute
    // - continues the loop, i.e. does not function like break
    // - can be nested into other statements
    // - only affects inner loop
    assert_env(R"(
        var x = 0;
        var y = 0;
        while y < 3 {
            while x < 2 {
                x = x + 1;
                if true { continue; }
                x = 100;
            }
            y = y + 1;
        })", {
        { "x", Lox::make_number(2) },
        { "y", Lox::make_number(3) },
    });

    assert_env(R"(
        var x = 0;
        var y = 0;
        for c in "foo" {
            for c in "bar" {
                x = x + 1;
                if true { continue; }
                x = 100;
            }
            y = y + 1;
        })", {
        { "x", Lox::make_number(9) },
        { "y", Lox::make_number(3) },
    });
}

TEST(Interpreter, Interrupt)
{
    Lox::g_interrupt = 1;
    assert_env("while true {}", {});
    // TODO for loop with infinite iterator

    Lox::g_interrupt = 1;
    assert_env("var x = 5;", {});
}

TEST(Interpreter, CallExpression)
{
    // no call - no side effects
    assert_env("var x = 1; fn f() { x = 2; }", {
        { "x", Lox::make_number(1) },
        { "f", std::make_shared<DummyFunction>(R"(
(fn
  f
  (params)
  (block
    (=
      x
      2))))") },
    });

    // call for side effects
    assert_env("fn f() { x = 2; } var x = 1; f();", {
        { "x", Lox::make_number(2) },
        { "f", std::make_shared<DummyFunction>(R"(
(fn
  f
  (params)
  (block
    (=
      x
      2))))") },
    });

    assert_error("5();", "5"); // not callable
}
