#include "AST.h"
#include <charconv>

namespace Lox {

static std::string make_indent(std::size_t indent)
{
    return std::string(indent * 2, ' ');
}

static std::string escape(std::string s)
{
    for (std::size_t i = 0; i < s.size(); ++i) {
        auto& ch = s[i];
        char sub = 0;
        switch (ch) {
        case '\t':
            sub = 't';
            break;
        case '\r':
            sub = 'r';
            break;
        case '\n':
            sub = 'n';
            break;
        case '"':
        case '\\':
            sub = ch;
            break;
        }
        if (sub > 0) {
            ch = '\\';
            s.insert(i + 1, 1, sub);
            ++i;
        }
    }
    return s.insert(0, 1, '"').append(1, '"');
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

}
