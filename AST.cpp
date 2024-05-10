#include "AST.h"
#include "Utils.h"
#include <charconv>

namespace Lox {

static std::string make_indent(std::size_t indent)
{
    return std::string(indent * 2, ' ');
}

std::string StringLiteral::dump(std::size_t indent) const
{
    return make_indent(indent).append(escape(m_value));
}

std::string NumberLiteral::dump(std::size_t indent) const
{
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_value);
    assert(ec == std::errc()); // longest double is 24 chars long
    return make_indent(indent).append(buf, ptr - buf);
}

std::string Identifier::dump(std::size_t indent) const
{
    return make_indent(indent).append(m_name);
}

std::string BoolLiteral::dump(std::size_t indent) const
{
    return make_indent(indent).append(m_value ? "true" : "false");
}

std::string NilLiteral::dump(std::size_t indent) const
{
    return make_indent(indent).append("nil");
}

std::string UnaryExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case UnaryOp::Minus:
        s += '-';
        break;
    case UnaryOp::Not:
        s += '!';
        break;
    }
    s += '\n';
    s += m_expr->dump(indent + 1);
    s += ')';
    return s;
}

std::string GroupExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(group\n";
    s += m_expr->dump(indent + 1);
    s += ')';
    return s;
}

std::string BinaryExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case BinaryOp::Divide:
        s += '/';
        break;
    case BinaryOp::Multiply:
        s += '*';
        break;
    case BinaryOp::Add:
        s += '+';
        break;
    case BinaryOp::Subtract:
        s += '-';
        break;
    case BinaryOp::Equal:
        s += "==";
        break;
    case BinaryOp::NotEqual:
        s += "!=";
        break;
    case BinaryOp::Less:
        s += '<';
        break;
    case BinaryOp::LessOrEqual:
        s += "<=";
        break;
    case BinaryOp::Greater:
        s += '>';
        break;
    case BinaryOp::GreaterOrEqual:
        s += ">=";
        break;
    }
    s += '\n';
    s += m_left->dump(indent + 1);
    s += '\n';
    s += m_right->dump(indent + 1);
    s += ')';
    return s;
}

std::string ExpressionStmt::dump(std::size_t indent) const
{
    return m_expr->dump(indent);
}

std::string VarStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(var\n";
    s += m_ident->dump(indent + 1);
    if (m_init) {
        s += '\n';
        s += m_init->dump(indent + 1);
    }
    s += ')';
    return s;
}

std::string PrintStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(print";
    if (m_expr) {
        s += '\n';
        s += m_expr->dump(indent + 1);
    }
    s += ')';
    return s;
}

std::string AssignStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(=\n";
    s += m_place->dump(indent + 1);
    s += '\n';
    s += m_value->dump(indent + 1);
    s += ')';
    return s;
}

std::string BlockStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(block";
    for (auto& stmt : m_stmts) {
        s += '\n';
        s += stmt->dump(indent + 1);
    }
    s += ')';
    return s;
}

std::string IfStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(if\n";
    s += m_test->dump(indent + 1);
    s += '\n';
    s += m_then_stmt->dump(indent + 1);
    if (m_else_stmt) {
        s += '\n';
        s += m_else_stmt->dump(indent + 1);
    }
    s += ')';
    return s;
}

std::string Program::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(program";
    for (auto& stmt : m_stmts) {
        s += '\n';
        s += stmt->dump(indent + 1);
    }
    s += ')';
    return s;
}

}
