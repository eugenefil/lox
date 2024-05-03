#include "Interpreter.h"
#include <format>

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
            interp.error(std::format(
                "cannot apply unary operator '-' to type '{}'",
                obj->type_name()));
            return {};
        }
        return std::make_shared<Number>(-obj->get_number());

    case UnaryOp::Not:
        return std::make_shared<Bool>(!obj->__bool__());
    }
    assert(0);
}

std::shared_ptr<Object> Interpreter::interpret()
{
    return m_ast->eval(*this);
}

}
