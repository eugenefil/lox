#pragma once

#include "Lexer.h"
#include "AST.h"
#include <vector>

namespace Lox {

class Parser {
public:
    explicit Parser(std::vector<Token>&& tokens)
    : m_tokens(std::move(tokens))
    {}

    std::shared_ptr<Program> parse();
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    const Token& peek() const;
    void advance() { ++m_cur; }

    void error(std::string msg, std::string_view span = {});

    std::shared_ptr<Expr> parse_primary();
    std::shared_ptr<Expr> parse_unary();
    std::shared_ptr<Expr> parse_multiply();
    std::shared_ptr<Expr> parse_add();
    std::shared_ptr<Expr> parse_compare();
    std::shared_ptr<Expr> parse_expression();

    std::shared_ptr<Stmt> parse_var_statement();
    std::shared_ptr<Stmt> parse_print_statement();
    std::shared_ptr<Stmt> parse_assign_statement(std::shared_ptr<Expr>);
    std::shared_ptr<Stmt> parse_statement();

    std::vector<Token> m_tokens;
    std::size_t m_cur { 0 };
    std::vector<Error> m_errors;
};

}
