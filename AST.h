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
    Modulo,
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

enum class LogicalOp {
    And,
    Or,
};

class LogicalExpr : public Expr {
public:
    LogicalExpr(LogicalOp op, std::shared_ptr<Expr> left,
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
    const LogicalOp m_op;
    std::shared_ptr<Expr> m_left;
    std::shared_ptr<Expr> m_right;
};

class CallExpr : public Expr {
public:
    explicit CallExpr(std::shared_ptr<Expr> callee,
        std::vector<std::shared_ptr<Expr>>&& args, std::string_view text)
        : Expr(text)
        , m_callee(callee)
        , m_args(std::move(args))
    {
        assert(callee);
        for (auto& arg : args)
            assert(arg);
    }

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

private:
    std::shared_ptr<Expr> m_callee;
    std::vector<std::shared_ptr<Expr>> m_args;
};

class BlockStmt;

class FunctionExpr: public Expr
    , public std::enable_shared_from_this<FunctionExpr> {
public:
    explicit FunctionExpr(std::vector<std::shared_ptr<Identifier>>&& params,
        std::shared_ptr<BlockStmt> block, std::string_view text)
        : Expr(text)
        , m_params(std::move(params))
        , m_block(block)
    {
        for (auto& param : params)
            assert(param);
        assert(block);
    }

    std::string dump(std::size_t indent) const override;
    std::shared_ptr<Object> eval(Interpreter&) const override;

    const std::vector<std::shared_ptr<Identifier>>& params() const
    {
        return m_params;
    }
    const BlockStmt& block() const { return *m_block; }

private:
    std::vector<std::shared_ptr<Identifier>> m_params;
    std::shared_ptr<BlockStmt> m_block;
};

class Stmt : public ASTNode {
public:
    explicit Stmt(std::string_view text) : ASTNode(text)
    {}

    virtual bool execute(Interpreter&) const = 0;
    virtual bool is_var_statement() const { return false; }
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
    bool execute(Interpreter&) const override;

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
        // initializer can be null
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;
    bool is_var_statement() const override { return true; }
    const Identifier& identifier() const { return *m_ident; }

private:
    std::shared_ptr<Identifier> m_ident;
    std::shared_ptr<Expr> m_init;
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
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Expr> m_place;
    std::shared_ptr<Expr> m_value;
};

class BlockStmt : public Stmt {
public:
    explicit BlockStmt(std::vector<std::shared_ptr<Stmt>>&& stmts,
                       std::string_view text)
        : Stmt(text)
        , m_stmts(std::move(stmts))
    {
        for (auto& stmt : stmts)
            assert(stmt);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

    const std::vector<std::shared_ptr<Stmt>>& statements() const { return m_stmts;}

private:
    std::vector<std::shared_ptr<Stmt>> m_stmts;
};

class IfStmt : public Stmt {
public:
    explicit IfStmt(std::shared_ptr<Expr> test, std::shared_ptr<Stmt> then_block,
                    std::shared_ptr<Stmt> else_block, std::string_view text)
        : Stmt(text)
        , m_test(test)
        , m_then_block(then_block)
        , m_else_block(else_block)
    {
        assert(test);
        assert(then_block);
        // 'else' block can be null
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Expr> m_test;
    std::shared_ptr<Stmt> m_then_block;
    std::shared_ptr<Stmt> m_else_block;
};

class WhileStmt : public Stmt {
public:
    explicit WhileStmt(std::shared_ptr<Expr> test, std::shared_ptr<Stmt> block,
                       std::string_view text)
        : Stmt(text)
        , m_test(test)
        , m_block(block)
    {
        assert(test);
        assert(block);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Expr> m_test;
    std::shared_ptr<Stmt> m_block;
};

class ForStmt : public Stmt {
public:
    explicit ForStmt(std::shared_ptr<Identifier> ident, std::shared_ptr<Expr> expr,
                     std::shared_ptr<BlockStmt> block, std::string_view text)
        : Stmt(text)
        , m_ident(ident)
        , m_expr(expr)
        , m_block(block)
    {
        assert(ident);
        assert(expr);
        assert(block);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Identifier> m_ident;
    std::shared_ptr<Expr> m_expr;
    std::shared_ptr<BlockStmt> m_block;
};

class BreakStmt : public Stmt {
public:
    explicit BreakStmt(std::string_view text) : Stmt(text)
    {}

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;
};

class ContinueStmt : public Stmt {
public:
    explicit ContinueStmt(std::string_view text) : Stmt(text)
    {}

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;
};

class FunctionDeclaration: public Stmt {
public:
    explicit FunctionDeclaration(std::shared_ptr<Identifier> name,
        std::shared_ptr<FunctionExpr> func, std::string_view text)
        : Stmt(text)
        , m_name(name)
        , m_func(func)
    {
        assert(name);
        assert(func);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Identifier> m_name;
    std::shared_ptr<FunctionExpr> m_func;
};

class ReturnStmt: public Stmt {
public:
    explicit ReturnStmt(std::shared_ptr<Expr> expr, std::string_view text)
        : Stmt(text)
        , m_expr(expr)
    {}

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::shared_ptr<Expr> m_expr; // can be null
};

class Program : public Stmt {
public:
    explicit Program(std::vector<std::shared_ptr<Stmt>>&& stmts,
                     std::string_view text)
        : Stmt(text)
        , m_stmts(std::move(stmts))
    {
        for (auto& stmt : stmts)
            assert(stmt);
    }

    std::string dump(std::size_t indent) const override;
    bool execute(Interpreter&) const override;

private:
    std::vector<std::shared_ptr<Stmt>> m_stmts;
};

}
