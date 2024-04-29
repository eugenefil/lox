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

std::string GroupExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += "(group\n";
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

std::string AddExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case AddOp::Add:
        s += '+';
        break;
    case AddOp::Subtract:
        s += '-';
        break;
    }
    s += '\n';
    s += m_left->dump(indent + 1);
    s += '\n';
    s += m_right->dump(indent + 1);
    s += ')';
    return s;
}

std::string CompareExpr::dump(std::size_t indent) const
{
    std::string s = make_indent(indent);
    s += '(';
    switch (m_op) {
    case CompareOp::Equal:
        s += "==";
        break;
    case CompareOp::NotEqual:
        s += "!=";
        break;
    case CompareOp::Less:
        s += '<';
        break;
    case CompareOp::LessOrEqual:
        s += "<=";
        break;
    case CompareOp::Greater:
        s += '>';
        break;
    case CompareOp::GreaterOrEqual:
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

void Parser::error(std::string_view msg)
{
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
                error("expected ')'");
        }
        return {};
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
        AddOp op = [&token]() {
            switch (token.type()) {
            case TokenType::Plus:
                return AddOp::Add;
            case TokenType::Minus:
                return AddOp::Subtract;
            default:
                assert(0);
            }
        }();
        left = std::make_shared<AddExpr>(op, left, right);
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
            CompareOp op = [&token]() {
                switch (token.type()) {
                case TokenType::EqualEqual:
                    return CompareOp::Equal;
                case TokenType::BangEqual:
                    return CompareOp::NotEqual;
                case TokenType::Less:
                    return CompareOp::Less;
                case TokenType::LessEqual:
                    return CompareOp::LessOrEqual;
                case TokenType::Greater:
                    return CompareOp::Greater;
                case TokenType::GreaterEqual:
                    return CompareOp::GreaterOrEqual;
                default:
                    assert(0);
                }
            }();
            return std::make_shared<CompareExpr>(op, left, right);
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
