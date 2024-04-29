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

static const Token EOF_TOKEN { TokenType::Eof, {} };

const Token& Parser::peek() const
{
    if (m_cur < m_tokens.size())
        return m_tokens[m_cur];
    return EOF_TOKEN;
}

bool Parser::match(TokenType type)
{
    if (peek().type() == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::error(std::string_view msg, std::string_view span)
{
    if (!span.empty()) {
        m_errors.push_back({ span, msg });
        return;
    }
    if (peek().type() == TokenType::Eof) {
        // w/out tokens there can't be errors, so there must be at least one token
        assert(m_tokens.size() > 0);
        m_errors.push_back({ m_tokens.back().text(), msg });
    } else
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
    } else if (token.type() == TokenType::LeftParen) {
        advance();
        if (auto expr = parse_expression()) {
            if (match(TokenType::RightParen))
                return std::make_shared<GroupExpr>(expr);
            else
                error("'(' was never closed", token.text());
        }
        return {};
    }
    error("expected expression");
    return {};
}

std::shared_ptr<Expr> Parser::parse_unary()
{
    if (auto& token = peek(); token.type() == TokenType::Minus ||
        token.type() == TokenType::Bang) {
        advance();
        if (auto expr = parse_unary()) {
            UnaryOp op = [&token]() {
                switch (token.type()) {
                case TokenType::Minus:
                    return UnaryOp::Minus;
                case TokenType::Bang:
                    return UnaryOp::Not;
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
        BinaryOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Slash:
                return BinaryOp::Divide;
            case TokenType::Star:
                return BinaryOp::Multiply;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<BinaryExpr>(op, left, right);
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_add()
{
    auto left = parse_multiply();
    if (!left)
        return {};

    for (;;) {
        auto& token = peek();
        if (!(token.type() == TokenType::Plus ||
            token.type() == TokenType::Minus))
            break;
        advance();
        auto right = parse_multiply();
        if (!right)
            return {};
        BinaryOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Plus:
                return BinaryOp::Add;
            case TokenType::Minus:
                return BinaryOp::Subtract;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<BinaryExpr>(op, left, right);
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_compare()
{
    auto left = parse_add();
    if (!left)
        return {};

    if (auto& token = peek(); token.type() == TokenType::EqualEqual ||
        token.type() == TokenType::BangEqual ||
        token.type() == TokenType::Less ||
        token.type() == TokenType::LessEqual ||
        token.type() == TokenType::Greater ||
        token.type() == TokenType::GreaterEqual) {
        advance();
        if (auto right = parse_add()) {
            BinaryOp op = [&token]() {
                switch (token.type()) {
                case TokenType::EqualEqual:
                    return BinaryOp::Equal;
                case TokenType::BangEqual:
                    return BinaryOp::NotEqual;
                case TokenType::Less:
                    return BinaryOp::Less;
                case TokenType::LessEqual:
                    return BinaryOp::LessOrEqual;
                case TokenType::Greater:
                    return BinaryOp::Greater;
                case TokenType::GreaterEqual:
                    return BinaryOp::GreaterOrEqual;
                default:
                    assert(0);
                }
            }();
            return std::make_shared<BinaryExpr>(op, left, right);
        }
        return {};
    }
    return left;
}

std::shared_ptr<Expr> Parser::parse_expression()
{
    return parse_compare();
}

std::shared_ptr<Expr> Parser::parse()
{
    if (m_tokens.empty())
        return {};
    return parse_expression();
}

}
