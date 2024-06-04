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
