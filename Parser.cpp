#include "Parser.h"

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
    }
    return {};
}

std::shared_ptr<Expr> Parser::parse()
{
    return parse_primary();
}

}
