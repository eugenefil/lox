#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <gtest/gtest.h>
#include <optional>

// dummy function that can be compared with the real user function object
// by comparing ast dumps, assuming non-empty dump is used for construction
// if empty dump is used for construction, dummy is considered equal
class DummyFunction : public Lox::Object {
public:
    DummyFunction() = default;
    explicit DummyFunction(std::string_view dump) : m_dump(dump)
    {
        while (!m_dump.starts_with('(')) {
            assert(!m_dump.empty());
            m_dump.remove_prefix(1);
        }
    }

    std::string_view type_name() const override { return "DummyFunction"; }
    std::string_view dump() const { return m_dump; }

private:
    std::string_view m_dump;
};

static std::shared_ptr<DummyFunction> make_dummy_function()
{
    return std::make_shared<DummyFunction>();
}

static void assert_scope_multi_program(std::vector<std::string_view> sources,
    const Lox::Scope::MapType& scope_vars,
    std::vector<std::optional<Lox::Error>> errors = {})
{
    if (errors.size() > 0) {
        ASSERT_EQ(errors.size(), sources.size());
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
        if (errors.empty() || !errors[i].has_value())
            ASSERT_FALSE(interp.has_errors());
        else {
            auto& errs = interp.errors();
            ASSERT_EQ(errs.size(), 1);
            EXPECT_EQ(errs[0].source, errors[i]->source);
            EXPECT_EQ(errs[0].span, errors[i]->span);
        }
    }

    auto& vars = interp.scope().vars();
    EXPECT_EQ(vars.size(), scope_vars.size());
    for (auto& [name, value] : scope_vars) {
        ASSERT_TRUE(vars.contains(name));
        auto& obj = vars.at(name);
        ASSERT_TRUE(obj);
        ASSERT_TRUE(value);
        if (value->type_name() == "DummyFunction") {
            ASSERT_EQ(obj->type_name(), "Function");
            auto& dummy = static_cast<DummyFunction&>(*value);
            if (!dummy.dump().empty()) {
                auto& func = static_cast<Lox::Function&>(*obj);
                EXPECT_EQ(func.ast().dump(0), dummy.dump());
            }
        } else
            EXPECT_TRUE(value->__eq__(*obj)) << '\n' <<
                "Variable: " << name << '\n' <<
                "  Actual: " << obj->__str__() << '\n' <<
                "Expected: " << value->__str__() << '\n';
    }
}

static void assert_scope(std::string_view source,
                         const Lox::Scope::MapType& scope_vars)
{
    assert_scope_multi_program({ source }, scope_vars);
}

static void assert_scope_and_error(std::string_view source,
                                   const Lox::Scope::MapType& scope_vars,
                                   std::string_view error_span)
{
    assert_scope_multi_program({ source }, scope_vars,
                             { Lox::Error { "", source, error_span } });
}

TEST(Interpreter, ProgramsShareGlobalScope)
{
    assert_scope_multi_program({ "var x = 5;", "var y = x * 2;" }, {
        { "x", Lox::make_number(5) },
        { "y", Lox::make_number(10) },
    });
}

TEST(Interpreter, GlobalScopeIsCurrentAfterError)
{
    // test that when error happens in the inner block, the scope stack
    // correctly "unwinds" and does not get stuck on the scope that was
    // current at the point of error - this is important for repl
    // below, error happens where y is defined, but on exit from the
    // interpreter the global scope must be current - where only x is defined
    assert_scope_and_error("var x = 1; { var y; foo; }",
        { { "x", Lox::make_number(1) } },
        "foo"
    );
}

TEST(Interpreter, Interrupt)
{
    Lox::g_interrupt = 1;
    assert_scope("while true {}", {});
    // TODO for loop with infinite iterator

    Lox::g_interrupt = 1;
    assert_scope("var x = 5;", {});
}

TEST(Interpreter, FunctionErrorHasFunctionSource)
{
    // in a repl a function can be defined while evaluating one source line
    // and called later while evaluating another; ensure that when an error
    // inside the function happens, it points to the function's original
    // definition source code and not the one of the function call site
    std::string_view definition = "fn f() { x; }";
    std::string_view call = "f();";
    assert_scope_multi_program({ definition, call },
        { { "f", std::make_shared<DummyFunction>() } }, {
        {},
        Lox::Error { "", definition, "x" },
    });
}

TEST(Interpreter, LexicalScope)
{
    // function's parent scope is determined by its source placement
    // rather than call sequence
    assert_scope(R"(
        var x = 5;
        fn g() { return x; }
        fn f() {
            var x = 7;
            return g();
        }
        var y = f();
        )", {
        { "x", Lox::make_number(5) },
        { "g", make_dummy_function() },
        { "f", make_dummy_function() },
        // with dynamic scope the parent scope of g() would be f()'s and
        // y would be 7; with static (lexical) scope the parent scope of
        // g() is global scope and y would be 5
        { "y", Lox::make_number(5) },
    });
}

TEST(Interpreter, Closure)
{
    // closure records all surrounding scopes during definition, can
    // access and modify their variables
    assert_scope(R"(
        fn f() {
            fn g(z) {
                x = x + z;
                y = y + z;
                return y;
            }
            var y = 7; // like with globals, var can be defined after function
            return g;
        }
        var x = 5; // global var can be defined after function that uses it
        var new_y = f()(10);
        )", {
        { "f", make_dummy_function() },
        { "x", Lox::make_number(15) },
        { "new_y", Lox::make_number(17) },
    });
}

TEST(Interpreter, FunctionExpression)
{
    // assign lambda to var and call later
    assert_scope("var f = fn(x) { return x; }; var y = f(5);", {
        { "f", make_dummy_function() },
        { "y", Lox::make_number(5) },
    });

    // lambda captures surrounding vars
    assert_scope(R"(
        var x = 1;
        var r = fn(y) { return fn(z) { return x + y + z; }; }(10)(100);
        )", {
        { "x", Lox::make_number(1) },
        { "r", Lox::make_number(111) },
    });

    // pass lambda to a function
    assert_scope(R"(
        var x = 1;
        fn modify(modifier) { x = modifier(x); }
        modify(fn(x) { return x + 10; });
        modify(fn(x) { return x * 2; });
    )", {
        { "modify", make_dummy_function() },
        { "x", Lox::make_number(22) },
    });
}
