#pragma once

#include <string>
#include <memory>
#include <cassert>
#include <vector>

namespace Lox {

class ASTNode {
public:
    virtual ~ASTNode() = default;

    explicit ASTNode(std::string_view text) : m_text(text)
    {}

    std::string_view text() const { return m_text; }
    virtual std::string dump(std::size_t indent) const = 0;

protected:
    std::string_view m_text;
};

class Object;
class Interpreter;

class Expr : public ASTNode {
public:
    virtual ~Expr() = default;

    explicit Expr(std::string_view text) : ASTNode(text)
    {}

    virtual std::shared_ptr<Object> eval(Interpreter&) const = 0;
    virtual bool is_identifier() const { return false; }
};

class StringLiteral : public Expr {
public:
    explicit StringLiteral(const std::string& value, std::string_view text)
        : Expr(text)
        , m_value(value)
    {}

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    std::string m_value;
};

class NumberLiteral : public Expr {
public:
    explicit NumberLiteral(double value, std::string_view text)
        : Expr(text)
        , m_value(value)
    {}

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    double m_value { 0.0 };
};

class Identifier : public Expr {
public:
    explicit Identifier(std::string_view name, std::string_view text)
        : Expr(text)
        , m_name(name)
    {}

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;
    bool is_identifier() const override { return true; }

    std::string_view name() const { return m_name; }

private:
    std::string_view m_name;
};

class BoolLiteral : public Expr {
public:
    explicit BoolLiteral(bool value, std::string_view text)
        : Expr(text)
        , m_value(value)
    {}

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    bool m_value { false };
};

class NilLiteral : public Expr {
public:
    explicit NilLiteral(std::string_view text) : Expr(text)
    {}

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;
};

enum class UnaryOp {
    Minus,
    Not,
};

class UnaryExpr : public Expr {
public:
    UnaryExpr(UnaryOp op, std::shared_ptr<Expr> expr, std::string_view text)
        : Expr(text)
        , m_op(op)
        , m_expr(expr)
    {
        assert(expr);
    }

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    const UnaryOp m_op;
    std::shared_ptr<Expr> m_expr;
};

class GroupExpr : public Expr {
public:
    GroupExpr(std::shared_ptr<Expr> expr, std::string_view text)
        : Expr(text)
        , m_expr(expr)
    {
        assert(expr);
    }

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

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
    BinaryExpr(BinaryOp op, std::shared_ptr<Expr> left,
        std::shared_ptr<Expr> right, std::string_view text)
        : Expr(text)
        , m_op(op)
        , m_left(left)
        , m_right(right)
    {
        assert(left);
        assert(right);
    }

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    const BinaryOp m_op;
    std::shared_ptr<Expr> m_left;
    std::shared_ptr<Expr> m_right;
};

class Stmt : public ASTNode {
public:
    explicit Stmt(std::string_view text) : ASTNode(text)
    {}

    virtual bool execute(Interpreter&) = 0;
};

class ExpressionStmt : public Stmt {
public:
    explicit ExpressionStmt(std::shared_ptr<Expr> expr, std::string_view text)
        : Stmt(text)
        , m_expr(expr)
    {
        assert(expr);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) override;

private:
    std::shared_ptr<Expr> m_expr;
};

class VarStmt : public Stmt {
public:
    explicit VarStmt(std::shared_ptr<Identifier> ident,
                     std::shared_ptr<Expr> init, std::string_view text)
        : Stmt(text)
        , m_ident(ident)
        , m_init(init)
    {
        assert(ident);
        // initializer may be null
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) override;

private:
    std::shared_ptr<Identifier> m_ident;
    std::shared_ptr<Expr> m_init;
};

class PrintStmt : public Stmt {
public:
    explicit PrintStmt(std::shared_ptr<Expr> expr, std::string_view text)
        : Stmt(text)
        , m_expr(expr)
    {}

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) override;

private:
    std::shared_ptr<Expr> m_expr;
};

class AssignStmt : public Stmt {
public:
    explicit AssignStmt(std::shared_ptr<Expr> place, std::shared_ptr<Expr> value,
                        std::string_view text)
        : Stmt(text)
        , m_place(place)
        , m_value(value)
    {
        assert(place);
        assert(place->is_identifier());
        assert(value);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) override;

private:
    std::shared_ptr<Expr> m_place;
    std::shared_ptr<Expr> m_value;
};

class Program : public Stmt {
public:
    explicit Program(std::vector<std::shared_ptr<Stmt>>&& stmts,
                     std::string_view text)
        : Stmt(text)
        , m_stmts(stmts)
    {
        for (auto& stmt : stmts)
            assert(stmt);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) override;

private:
    std::vector<std::shared_ptr<Stmt>> m_stmts;
};

}
