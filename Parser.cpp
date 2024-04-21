#include "Parser.h"
#include <cassert>
#include <charconv>

namespace Lox {

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

std::string StringLiteral::dump() const
{
    return escape(m_value);
}

std::string NumberLiteral::dump() const
{
    char buf[32];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), m_value);
    assert(ec == std::errc()); // longest double is 24 chars long
    return std::string(buf, ptr - buf);
}

static const Token EOF_TOKEN { Token::Type::Eof, "" };

const Token& Parser::peek() const
{
    if (m_cur < m_tokens.size())
        return m_tokens[m_cur];
    return EOF_TOKEN;
}

std::shared_ptr<Expr> Parser::parse_primary()
{
    auto& token = peek();
    if (token.type() == Token::Type::String) {
        advance();
        return std::make_shared<StringLiteral>(std::get<std::string>(token.value()));
    } else if (token.type() == Token::Type::Number) {
        advance();
        return std::make_shared<NumberLiteral>(std::get<double>(token.value()));
    } else if (token.type() == Token::Type::True ||
               token.type() == Token::Type::False) {
        advance();
        return std::make_shared<BoolLiteral>(std::get<bool>(token.value()));
    } else if (token.type() == Token::Type::Nil) {
        advance();
        return std::make_shared<NilLiteral>();
    }
    return {};
}

std::shared_ptr<Expr> Parser::parse()
{
    return parse_primary();
}

}
