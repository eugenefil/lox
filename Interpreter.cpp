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

std::string Number::__str__() const
{
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_value);
    assert(ec == std::errc()); // longest double is 24 chars long
    return std::string(buf, ptr - buf);
}

void Interpreter::error(std::string msg, std::string_view span)
{
    m_errors.push_back({ span, std::move(msg) });
}

std::shared_ptr<Object> Interpreter::interpret()
{
    if (!m_ast)
        return {};
    return m_ast->eval(*this);
}

}
