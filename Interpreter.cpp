#include "Interpreter.h"
#include <format>
#include <charconv>
#include <iostream>
#include <cmath>

namespace Lox {

class Iterator {
public:
    virtual bool done() const = 0;
    virtual std::shared_ptr<Object> next() = 0;
};

class StringIterator : public Iterator {
public:
    explicit StringIterator(std::shared_ptr<const String> str) : m_str(str)
    {
        assert(str);
    }

    bool done() const override { return m_pos >= m_str->size(); }

    std::shared_ptr<Object> next() override
    {
        assert(!done());
        return std::make_shared<String>(m_str->get_char(m_pos++));
    }

private:
    std::shared_ptr<const String> m_str;
    std::size_t m_pos { 0 };
};

std::shared_ptr<Iterator> String::__iter__() const
{
    return std::make_shared<StringIterator>(shared_from_this());
}

std::string Number::__str__() const
{
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_value);
    assert(ec == std::errc()); // longest double is 24 chars long
    return std::string(buf, ptr - buf);
}

std::shared_ptr<Object> Function::__call__(Interpreter& interp)
{
    if (!m_decl->block().execute(interp))
        return {};
    return {};
}

std::shared_ptr<Object> StringLiteral::eval(Interpreter&) const
{
    return make_string(m_value);
}

std::shared_ptr<Object> NumberLiteral::eval(Interpreter&) const
{
    return make_number(m_value);
}

std::shared_ptr<Object> Identifier::eval(Interpreter& interp) const
{
    if (auto val = interp.get_var(m_name))
        return val;
    interp.error(std::format("variable '{}' is not defined", m_name), m_text);
    return {};
}

std::shared_ptr<Object> BoolLiteral::eval(Interpreter&) const
{
    return make_bool(m_value);
}

std::shared_ptr<Object> NilLiteral::eval(Interpreter&) const
{
    return make_nil();
}

std::shared_ptr<Object> UnaryExpr::eval(Interpreter& interp) const
{
    auto obj = m_expr->eval(interp);
    if (!obj)
        return {};

    switch (m_op) {
    case UnaryOp::Minus:
        if (!obj->is_number()) {
            interp.error(std::format("cannot apply unary operator '-' to type '{}'",
                obj->type_name()), m_text);
            return {};
        }
        return make_number(-obj->get_number());

    case UnaryOp::Not:
        if (!obj->is_bool()) {
            interp.error(std::format("cannot apply unary operator '!' to type '{}'",
                obj->type_name()), m_text);
            return {};
        }
        return make_bool(!obj->get_bool());
    }
    assert(0);
}

std::shared_ptr<Object> GroupExpr::eval(Interpreter& interp) const
{
    return m_expr->eval(interp);
}

std::shared_ptr<Object> BinaryExpr::eval(Interpreter& interp) const
{
    auto left = m_left->eval(interp);
    if (!left)
        return {};
    auto right = m_right->eval(interp);
    if (!right)
        return {};

    switch (m_op) {
    case BinaryOp::Divide:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() / right->get_number());
        interp.error(std::format("cannot divide '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Multiply:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() * right->get_number());
        interp.error(std::format("cannot multiply '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Modulo:
        if (left->is_number() && right->is_number())
            return make_number(std::fmod(left->get_number(), right->get_number()));
        interp.error(std::format("cannot divide '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Add:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() + right->get_number());
        else if (left->is_string() && right->is_string())
            return make_string(std::string(left->get_string())
                .append(right->get_string()));
        interp.error(std::format("cannot add '{}' to '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Subtract:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() - right->get_number());
        interp.error(std::format("cannot subtract '{}' from '{}'",
            right->type_name(), left->type_name()), m_text);
        return {};

    case BinaryOp::Equal:
        if (left->type_name() == right->type_name())
            return make_bool(left->__eq__(*right));
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::NotEqual:
        if (left->type_name() == right->type_name())
            return make_bool(!left->__eq__(*right));
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Less:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() < right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() < right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::LessOrEqual:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() <= right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() <= right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Greater:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() > right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() > right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::GreaterOrEqual:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() >= right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() >= right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};
    };
    assert(0);
}

std::shared_ptr<Object> LogicalExpr::eval(Interpreter& interp) const
{
    auto left = m_left->eval(interp);
    if (!left)
        return {};
    if (!left->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", left->type_name()),
            m_left->text());
        return {};
    }

    switch (m_op) {
    case LogicalOp::And:
        if (!left->get_bool())
            return make_bool(false);
        break;
    case LogicalOp::Or:
        if (left->get_bool())
            return make_bool(true);
        break;
    default:
        assert(0);
    }

    auto right = m_right->eval(interp);
    if (!right)
        return {};
    if (!right->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", right->type_name()),
            m_right->text());
        return {};
    }
    return make_bool(right->get_bool());
}

std::shared_ptr<Object> CallExpr::eval(Interpreter& interp) const
{
    auto val = m_callee->eval(interp);
    if (!val)
        return {};
    if (!val->is_function()) {
        interp.error(std::format("'{}' object is not callable", val->type_name()),
            m_callee->text());
        return {};
    }
    return val->__call__(interp);
}

bool ExpressionStmt::execute(Interpreter& interp)
{
    if (auto val = m_expr->eval(interp)) {
        if (interp.is_repl_mode()) {
            auto str = val->__str__();
            if (val->is_string())
                str = escape(str);
            std::cout << str << '\n';
        }
        return true;
    }
    return false;
}

bool VarStmt::execute(Interpreter& interp)
{
    std::shared_ptr<Object> val;
    if (m_init) {
        val = m_init->eval(interp);
        if (!val)
            return false;
    } else
        val = make_nil();
    assert(val);
    interp.define_var(m_ident->name(), val);
    return true;
}

bool PrintStmt::execute(Interpreter& interp)
{
    if (m_expr) {
        auto val = m_expr->eval(interp);
        if (!val)
            return false;
        std::cout << val->__str__();
    }
    std::cout << '\n';
    return true;
}

bool AssignStmt::execute(Interpreter& interp)
{
    auto val = m_value->eval(interp);
    if (!val)
        return false;

    std::string_view name;
    if (m_place->is_identifier())
        name = std::static_pointer_cast<Identifier>(m_place)->name();
    assert(!name.empty());

    if (interp.set_var(name, val))
        return true;

    if (m_place->is_identifier())
        interp.error(std::format("variable '{}' is not defined", name),
                     m_place->text());
    else
        assert(0);
    return false;
}

void BlockStmt::inject_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(name.size());
    assert(value);
    m_injected_name = name;
    m_injected_value = value;
}

bool BlockStmt::execute(Interpreter& interp)
{
    interp.push_env();
    if (!m_injected_name.empty()) {
        interp.define_var(m_injected_name, std::move(m_injected_value));
        m_injected_name = {};
    }

    for (auto& stmt : m_stmts) {
        if (!stmt->execute(interp)) {
            interp.pop_env();
            return false;
        }
    }
    interp.pop_env();
    return true;
}

bool IfStmt::execute(Interpreter& interp)
{
    auto val = m_test->eval(interp);
    if (!val)
        return false;
    if (!val->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", val->type_name()),
            m_test->text());
        return false;
    }

    if (val->get_bool())
        return m_then_block->execute(interp);
    if (m_else_block)
        return m_else_block->execute(interp);
    return true;
}

bool WhileStmt::execute(Interpreter& interp)
{
    for (;;) {
        if (interp.check_interrupt())
            return false;

        auto val = m_test->eval(interp);
        if (!val)
            return false;
        if (!val->is_bool()) {
            interp.error(std::format("expected 'Bool', got '{}'",
                val->type_name()), m_test->text());
            return false;
        }
        if (!val->get_bool())
            break;

        assert(!interp.is_break());
        assert(!interp.is_continue());
        if (!m_block->execute(interp)) {
            if (interp.is_break()) {
                interp.set_break(false);
                break;
            }
            if (interp.is_continue()) {
                interp.set_continue(false);
                continue;
            }
            return false;
        }
    }
    return true;
}

bool ForStmt::execute(Interpreter& interp)
{
    auto val = m_expr->eval(interp);
    if (!val)
        return false;

    if (!val->is_iterable()) {
        interp.error(std::format("'{}' is not iterable", val->type_name()),
                     m_expr->text());
        return false;
    }

    auto iter = val->__iter__();
    assert(iter);

    while (!iter->done()) {
        auto next = iter->next();
        if (!next)
            return false;

        m_block->inject_var(m_ident->name(), next);
        assert(!interp.is_break());
        assert(!interp.is_continue());
        if (!m_block->execute(interp)) {
            if (interp.is_break()) {
                interp.set_break(false);
                break;
            }
            if (interp.is_continue()) {
                interp.set_continue(false);
                continue;
            }
            return false;
        }
    }
    return true;
}

bool BreakStmt::execute(Interpreter& interp)
{
    interp.set_break(true);
    return false;
}

bool ContinueStmt::execute(Interpreter& interp)
{
    interp.set_continue(true);
    return false;
}

bool FunctionDeclaration::execute(Interpreter& interp)
{
    interp.define_var(m_name->name(),
                      std::make_shared<Function>(shared_from_this()));
    return true;
}

bool Program::execute(Interpreter& interp)
{
    for (auto& stmt : m_stmts) {
        if (interp.check_interrupt())
            return false;
        if (!stmt->execute(interp))
            return false;
    }
    return true;
}

void Interpreter::push_env()
{
    m_env_stack.emplace_front();
}

void Interpreter::pop_env()
{
    assert(!m_env_stack.empty());
    m_env_stack.pop_front();
    assert(!m_env_stack.empty()); // global env must always exist
}

void Interpreter::define_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(!name.empty());
    assert(value);
    assert(!m_env_stack.empty());
    m_env_stack.front()[name] = value;
}

std::shared_ptr<Object> Interpreter::get_var(std::string_view name) const
{
    assert(!name.empty());
    assert(!m_env_stack.empty());

    for (auto& env : m_env_stack) {
        if (auto pair = env.find(name); pair != env.end())
            return pair->second;
    }
    return {};
}

bool Interpreter::set_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(!name.empty());
    assert(value);
    assert(!m_env_stack.empty());

    for (auto& env : m_env_stack) {
        if (auto pair = env.find(name); pair != env.end()) {
            pair->second = value;
            return true;
        }
    }
    return false;
}

void Interpreter::error(std::string msg, std::string_view span)
{
    m_errors.push_back({ std::move(msg), span });
}

void Interpreter::interpret(std::shared_ptr<Program> program)
{
    m_errors.clear();
    assert(program);
    assert(m_env_stack.size() == 1);
    program->execute(*this);
    assert(m_env_stack.size() == 1);
}

volatile std::sig_atomic_t g_interrupt;

bool Interpreter::check_interrupt()
{
    if (g_interrupt) {
        g_interrupt = 0;
        std::cerr << "interrupt\n";
        // when infinite loop does output to tty std::cout and SIGINT is
        // issued to stop that, failbit and badbit get set; clear those
        std::cout.clear();
        return true;
    }
    return false;
}

}
