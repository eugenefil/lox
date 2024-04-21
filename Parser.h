#include "Scanner.h"
#include <vector>
#include <memory>

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

class Parser {
public:
    explicit Parser(std::vector<Token>&& tokens)
    : m_tokens(std::move(tokens))
    {}

    std::shared_ptr<Expr> parse();

private:
    const Token& peek() const;
    void advance() { ++m_cur; }

    std::shared_ptr<Expr> parse_primary();

    std::vector<Token> m_tokens;
    std::size_t m_cur { 0 };
};

}
