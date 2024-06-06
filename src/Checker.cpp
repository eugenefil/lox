#include "Checker.h"
#include <cassert>

namespace Lox {

class ScopePusher {
public:
    ScopePusher(Checker& checker) : m_checker(checker)
    {
        m_checker.push_scope();
    }

    ~ScopePusher()
    {
        m_checker.pop_scope();
    }

private:
    Checker& m_checker;
};

static bool check_statements(const std::vector<std::shared_ptr<Stmt>>& stmts,
    Checker& checker)
{
    for (auto& stmt : stmts) {
        if (!stmt->check(checker))
            return false;
    }
    return true;
}

bool Identifier::check(Checker& checker)
{
    m_hops = checker.hops_to_name(m_name);
    return true;
}

bool UnaryExpr::check(Checker& checker)
{
    return m_expr->check(checker);
}

bool GroupExpr::check(Checker& checker)
{
    return m_expr->check(checker);
}

bool BinaryExpr::check(Checker& checker)
{
    return m_left->check(checker) && m_right->check(checker);
}

bool LogicalExpr::check(Checker& checker)
{
    return m_left->check(checker) && m_right->check(checker);
}

bool CallExpr::check(Checker& checker)
{
    if (!m_callee->check(checker))
        return false;
    for (auto& arg : m_args) {
        if (!arg->check(checker))
            return false;
    }
    return true;
}

bool FunctionExpr::check(Checker& checker)
{
    ScopePusher new_scope(checker);
    for (auto& param : m_params)
        checker.declare(param->name());
    return check_statements(m_block->statements(), checker);
}

bool ExpressionStmt::check(Checker& checker)
{
    return m_expr->check(checker);
}

bool AssertStmt::check(Checker& checker)
{
    return m_expr->check(checker);
}

bool VarStmt::check(Checker& checker)
{
    if (m_init && !m_init->check(checker))
        return false;
    checker.declare(m_ident->name());
    return true;
}

bool AssignStmt::check(Checker& checker)
{
    return m_place->check(checker) && m_value->check(checker);
}

bool BlockStmt::check(Checker& checker)
{
    ScopePusher new_scope(checker);
    return check_statements(m_stmts, checker);
}

bool IfStmt::check(Checker& checker)
{
    if (!m_test->check(checker))
        return false;
    if (!m_then_block->check(checker))
        return false;
    if (m_else_block)
        return m_else_block->check(checker);
    return true;
}

bool WhileStmt::check(Checker& checker)
{
    if (!m_test->check(checker))
        return false;
    return m_block->check(checker);
}

bool ForStmt::check(Checker& checker)
{
    if (!m_expr->check(checker))
        return false;
    ScopePusher new_scope(checker);
    checker.declare(m_ident->name());
    return check_statements(m_block->statements(), checker);
}

bool FunctionDeclaration::check(Checker& checker)
{
    checker.declare(m_name->name()); 
    return m_func->check(checker);
}

bool ReturnStmt::check(Checker& checker)
{
    if (m_expr)
        return m_expr->check(checker);
    return true;
}

bool Program::check(Checker& checker)
{
    ScopePusher new_scope(checker);
    return check_statements(m_stmts, checker);
}

void Checker::push_scope()
{
    m_scope_stack.emplace_front();
}

void Checker::pop_scope()
{
    assert(m_scope_stack.size());
    m_scope_stack.pop_front();
}

void Checker::declare(std::string_view name)
{
    assert(m_scope_stack.size());
    m_scope_stack.front()[name] = true;
}

std::optional<std::size_t> Checker::hops_to_name(std::string_view name)
{
    std::size_t hops = 0;
    for (auto& scope : m_scope_stack) {
        if (scope.contains(name))
            return { hops };
        ++hops;
    }
    return {};
}

void Checker::error(std::string msg, std::string_view span)
{
    m_errors.push_back({ std::move(msg), m_source, span });
}

void Checker::check(const std::shared_ptr<Program>& program)
{
    assert(program);
    TemporaryChange<std::string_view> new_source(m_source, program->text());
    program->check(*this);
}

}
