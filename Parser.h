#pragma once

#include "Lexer.h"
#include <vector>
#include <memory>
#include <cassert>

namespace Lox {

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::string dump() const = 0;
};

class StringLiteral : public Expr {
public:
    explicit StringLiteral(const std::string& value) : m_value(value)
    {}

    std::string dump() const override;

private:
    std::string m_value;
};

class NumberLiteral : public Expr {
public:
    explicit NumberLiteral(double value) : m_value(value)
    {}

    std::string dump() const override;

private:
    double m_value { 0.0 };
};

class BoolLiteral : public Expr {
public:
    explicit BoolLiteral(bool value) : m_value(value)
    {}

    std::string dump() const override { return m_value ? "true" : "false"; }

private:
    bool m_value { false };
};

class NilLiteral : public Expr {
public:
    std::string dump() const override { return "nil"; }
};

enum class UnaryOp {
    Minus,
    Plus,
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(UnaryOp op, std::shared_ptr<Expr> expr)
        : m_op(op)
        , m_expr(expr)
    {
        assert(expr);
    }

    std::string dump() const override;

private:
    const UnaryOp m_op;
    std::shared_ptr<Expr> m_expr;
};

class Parser {
public:
    explicit Parser(std::vector<Token>&& tokens)
    : m_tokens(std::move(tokens))
    {}

    std::shared_ptr<Expr> parse();
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    const Token& peek() const;
    void advance() { ++m_cur; }

    std::shared_ptr<Expr> parse_primary();
    std::shared_ptr<Expr> parse_unary();

    std::vector<Token> m_tokens;
    std::size_t m_cur { 0 };
    std::vector<Error> m_errors;
};

}
