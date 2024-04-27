#include "Parser.h"
#include <cassert>
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
    case UnaryOp::Plus:
        s += '+';
        break;
    }
    s += '\n';
    s += m_expr->dump(indent + 1);
    s += ')';
    return s;
}

std::string MultiplyExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case MultiplyOp::Divide:
        s += '/';
        break;
    case MultiplyOp::Multiply:
        s += '*';
        break;
    }
    s += '\n';
    s += m_left->dump(indent + 1);
    s += '\n';
    s += m_right->dump(indent + 1);
    s += ')';
    return s;
}

static const Token EOF_TOKEN { TokenType::Eof, {} };

const Token& Parser::peek() const
{
    if (m_cur < m_tokens.size())
        return m_tokens[m_cur];
    return EOF_TOKEN;
}

void Parser::error(std::string_view msg)
{
    m_errors.push_back({ peek().text(), msg });
}

std::shared_ptr<Expr> Parser::parse_primary()
{
    auto& token = peek();
    if (token.type() == TokenType::String) {
        advance();
        return std::make_shared<StringLiteral>(std::get<std::string>(token.value()));
    } else if (token.type() == TokenType::Number) {
        advance();
        return std::make_shared<NumberLiteral>(std::get<double>(token.value()));
    } else if (token.type() == TokenType::Identifier) {
        advance();
        return std::make_shared<Identifier>(token.text());
    } else if (token.type() == TokenType::True ||
               token.type() == TokenType::False) {
        advance();
        return std::make_shared<BoolLiteral>(std::get<bool>(token.value()));
    } else if (token.type() == TokenType::Nil) {
        advance();
        return std::make_shared<NilLiteral>();
    }
    error("expected expression");
    return {};
}

std::shared_ptr<Expr> Parser::parse_unary()
{
    if (auto& token = peek(); token.type() == TokenType::Minus ||
        token.type() == TokenType::Plus) {
        advance();
        if (auto expr = parse_unary()) {
            UnaryOp op = [&token]() {
                switch (token.type()) {
                case TokenType::Minus:
                    return UnaryOp::Minus;
                case TokenType::Plus:
                    return UnaryOp::Plus;
                default:
                    assert(0);
                }
            }();
            return std::make_shared<UnaryExpr>(op, expr);
        }
        return {};
    }
    return parse_primary();
}

std::shared_ptr<Expr> Parser::parse_multiply()
{
    auto left = parse_unary();
    if (!left)
        return {};

    for (;;) {
        auto& token = peek();
        if (!(token.type() == TokenType::Slash ||
            token.type() == TokenType::Star))
            break;
        advance();
        auto right = parse_unary();
        if (!right)
            return {};
        MultiplyOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Slash:
                return MultiplyOp::Divide;
            case TokenType::Star:
                return MultiplyOp::Multiply;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<MultiplyExpr>(op, left, right);
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse()
{
    if (m_tokens.empty())
        return {};
    return parse_multiply();
}

}
