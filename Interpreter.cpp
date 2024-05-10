#include "Interpreter.h"
#include <format>
#include <charconv>
#include <iostream>

namespace Lox {

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
        return make_bool(!obj->__bool__());
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

std::string Number::__str__() const
{
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_value);
    assert(ec == std::errc()); // longest double is 24 chars long
    return std::string(buf, ptr - buf);
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

bool BlockStmt::execute(Interpreter& interp)
{
    interp.push_env();
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

    if (val->__bool__())
        return m_then_stmt->execute(interp);
    if (m_else_stmt)
        return m_else_stmt->execute(interp);
    return true;
}

bool Program::execute(Interpreter& interp)
{
    for (auto& stmt : m_stmts) {
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
    m_env_stack.front()[std::string(name)] = value;
}

std::shared_ptr<Object> Interpreter::get_var(std::string_view name) const
{
    assert(!name.empty());
    assert(!m_env_stack.empty());

    std::string strname(name);
    for (auto& env : m_env_stack) {
        if (auto pair = env.find(strname); pair != env.end())
            return pair->second;
    }
    return {};
}

bool Interpreter::set_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(!name.empty());
    assert(value);
    assert(!m_env_stack.empty());

    std::string strname(name);
    for (auto& env : m_env_stack) {
        if (auto pair = env.find(strname); pair != env.end()) {
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

}
