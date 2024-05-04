#include "Interpreter.h"
#include <format>
#include <charconv>

namespace Lox {

std::shared_ptr<Object> Expr::eval(Interpreter&) const
{
    assert(0);
}

std::shared_ptr<Object> StringLiteral::eval(Interpreter&) const
{
    return std::make_shared<String>(m_value);
}

std::shared_ptr<Object> NumberLiteral::eval(Interpreter&) const
{
    return std::make_shared<Number>(m_value);
}

std::shared_ptr<Object> BoolLiteral::eval(Interpreter&) const
{
    return std::make_shared<Bool>(m_value);
}

std::shared_ptr<Object> NilLiteral::eval(Interpreter&) const
{
    return std::make_shared<NilType>();
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
        return std::make_shared<Number>(-obj->get_number());

    case UnaryOp::Not:
        return std::make_shared<Bool>(!obj->__bool__());
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
            return std::make_shared<Number>(left->get_number() /
                                            right->get_number());
        interp.error(std::format("cannot divide '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Multiply:
        if (left->is_number() && right->is_number())
            return std::make_shared<Number>(left->get_number() *
                                            right->get_number());
        interp.error(std::format("cannot multiply '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Add:
        if (left->is_number() && right->is_number())
            return std::make_shared<Number>(left->get_number() +
                                            right->get_number());
        else if (left->is_string() && right->is_string())
            return std::make_shared<String>(
                std::string(left->get_string()).append(right->get_string()));
        interp.error(std::format("cannot add '{}' to '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Subtract:
        if (left->is_number() && right->is_number())
            return std::make_shared<Number>(left->get_number() -
                                            right->get_number());
        interp.error(std::format("cannot subtract '{}' from '{}'",
            right->type_name(), left->type_name()), m_text);
        return {};

    case BinaryOp::Equal:
        return std::make_shared<Bool>(left->__eq__(*right));
    case BinaryOp::NotEqual:
        return std::make_shared<Bool>(!left->__eq__(*right));

    case BinaryOp::Less:
        if (left->is_number() && right->is_number())
            return std::make_shared<Bool>(left->get_number() <
                                          right->get_number());
        else if (left->is_string() && right->is_string())
            return std::make_shared<Bool>(left->get_string() <
                                          right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::LessOrEqual:
        if (left->is_number() && right->is_number())
            return std::make_shared<Bool>(left->get_number() <=
                                          right->get_number());
        else if (left->is_string() && right->is_string())
            return std::make_shared<Bool>(left->get_string() <=
                                          right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Greater:
        if (left->is_number() && right->is_number())
            return std::make_shared<Bool>(left->get_number() >
                                          right->get_number());
        else if (left->is_string() && right->is_string())
            return std::make_shared<Bool>(left->get_string() >
                                          right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::GreaterOrEqual:
        if (left->is_number() && right->is_number())
            return std::make_shared<Bool>(left->get_number() >=
                                          right->get_number());
        else if (left->is_string() && right->is_string())
            return std::make_shared<Bool>(left->get_string() >=
                                          right->get_string());
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

void Interpreter::error(std::string msg, std::string_view span)
{
    m_errors.push_back({ std::move(msg), span });
}

std::shared_ptr<Object> Interpreter::interpret()
{
    if (!m_ast)
        return {};
    return m_ast->eval(*this);
}

}
