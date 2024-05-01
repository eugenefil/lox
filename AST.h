#pragma once

#include <string>
#include <memory>
#include <cassert>

namespace Lox {

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::string dump(std::size_t indent) const = 0;
};

class StringLiteral : public Expr {
public:
    explicit StringLiteral(const std::string& value) : m_value(value)
    {}

    std::string dump(std::size_t indent) const override;

private:
    std::string m_value;
};

class NumberLiteral : public Expr {
public:
    explicit NumberLiteral(double value) : m_value(value)
    {}

    std::string dump(std::size_t indent) const override;

private:
    double m_value { 0.0 };
};

class Identifier : public Expr {
public:
    explicit Identifier(std::string_view name) : m_name(name)
    {}

    std::string dump(std::size_t indent) const override;

private:
    std::string_view m_name;
};

class BoolLiteral : public Expr {
public:
    explicit BoolLiteral(bool value) : m_value(value)
    {}

    std::string dump(std::size_t indent) const override;

private:
    bool m_value { false };
};

class NilLiteral : public Expr {
public:
    std::string dump(std::size_t indent) const override;
};

enum class UnaryOp {
    Minus,
    Not,
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(UnaryOp op, std::shared_ptr<Expr> expr)
        : m_op(op)
        , m_expr(expr)
    {
        assert(expr);
    }

    std::string dump(std::size_t indent) const override;

private:
    const UnaryOp m_op;
    std::shared_ptr<Expr> m_expr;
};

class GroupExpr : public Expr {
public:
    GroupExpr(std::shared_ptr<Expr> expr) : m_expr(expr)
    {
        assert(expr);
    }

    std::string dump(std::size_t indent) const override;

private:
    std::shared_ptr<Expr> m_expr;
};

enum class BinaryOp {
    Divide,
    Multiply,
    Add,
    Subtract,
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
};

class BinaryExpr : public Expr {
public:
    BinaryExpr(BinaryOp op, std::shared_ptr<Expr> left, std::shared_ptr<Expr> right)
        : m_op(op)
        , m_left(left)
        , m_right(right)
    {
        assert(left);
        assert(right);
    }

    std::string dump(std::size_t indent) const override;

private:
    const BinaryOp m_op;
    std::shared_ptr<Expr> m_left;
    std::shared_ptr<Expr> m_right;
};

}
