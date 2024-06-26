#include "AST.h"
#include "Utils.h"

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
    return make_indent(indent).append(number_to_string(m_value));
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
    case BinaryOp::Modulo:
        s += '%';
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

std::string LogicalExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case LogicalOp::And:
        s += "and";
        break;
    case LogicalOp::Or:
        s += "or";
        break;
    default:
        assert(0);
    }
    s += '\n';
    s += m_left->dump(indent + 1);
    s += '\n';
    s += m_right->dump(indent + 1);
    s += ')';
    return s;
}

std::string CallExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(call\n";
    s += m_callee->dump(indent + 1);
    s += '\n';
    s += make_indent(indent + 1);
    s += "(args";
    for (auto& arg : m_args) {
        s += '\n';
        s += arg->dump(indent + 2);
    }
    s += "))";
    return s;
}

std::string FunctionExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(fn\n";
    s += make_indent(indent + 1);
    s += "(params";
    for (auto& param : m_params) {
        s += '\n';
        s += param->dump(indent + 2);
    }
    s += ")\n";
    s += m_block->dump(indent + 1);
    s += ')';
    return s;
}

std::string ExpressionStmt::dump(std::size_t indent) const
{
    return m_expr->dump(indent);
}

std::string AssertStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(assert\n";
    s += m_expr->dump(indent + 1);
    s += ')';
    return s;
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
    s += m_then_block->dump(indent + 1);
    if (m_else_block) {
        s += '\n';
        s += m_else_block->dump(indent + 1);
    }
    s += ')';
    return s;
}

std::string WhileStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(while\n";
    s += m_test->dump(indent + 1);
    s += '\n';
    s += m_block->dump(indent + 1);
    s += ')';
    return s;
}

std::string ForStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(for\n";
    s += m_ident->dump(indent + 1);
    s += '\n';
    s += m_expr->dump(indent + 1);
    s += '\n';
    s += m_block->dump(indent + 1);
    s += ')';
    return s;
}

std::string BreakStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(break)";
    return s;
}

std::string ContinueStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(continue)";
    return s;
}

std::string FunctionDeclaration::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(fndecl\n";
    s += m_name->dump(indent + 1);
    s += '\n';
    s += make_indent(indent + 1);
    s += "(params";
    for (auto& param : m_func->params()) {
        s += '\n';
        s += param->dump(indent + 2);
    }
    s += ")\n";
    s += m_func->block().dump(indent + 1);
    s += ')';
    return s;
}

std::string ReturnStmt::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(return";
    if (m_expr) {
        s += '\n';
        s += m_expr->dump(indent + 1);
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
