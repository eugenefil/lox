#include "Scanner.h"
#include <cassert>

namespace Lox {

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.text();
    return out;
}

char Scanner::peek() const
{
    return m_end < m_input.size() ? m_input[m_end] : 0;
}

bool Scanner::match(char next)
{
    if (m_end < m_input.size() && m_input[m_end] == next) {
        ++m_end;
        return true;
    }
    return false;
}

constexpr bool is_ascii_alpha(char ch)
{
    return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z');
}

constexpr bool is_ascii_digit(char ch)
{
    return '0' <= ch && ch <= '9';
}

constexpr bool is_identifier_first_char(char ch)
{
    return is_ascii_alpha(ch) || ch == '_';
}

constexpr bool is_identifier_char(char ch)
{
    return is_identifier_first_char(ch) || is_ascii_digit(ch);
}

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;

    auto add_token = [&](Token::Type type) {
            assert(m_end > m_beg);
            tokens.push_back(Token { type, m_input.substr(m_beg, m_end - m_beg) });
            m_beg = m_end;
    };

    while (m_beg < m_input.size()) {
        auto ch = m_input[m_beg];
        m_end = m_beg + 1;
        switch (ch) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            m_beg = m_end;
            break;
        case '(':
            add_token(Token::Type::LeftParen);
            break;
        case ')':
            add_token(Token::Type::RightParen);
            break;
        case '{':
            add_token(Token::Type::LeftBrace);
            break;
        case '}':
            add_token(Token::Type::RightBrace);
            break;
        case ',':
            add_token(Token::Type::Comma);
            break;
        case '.':
            add_token(Token::Type::Dot);
            break;
        case '-':
            add_token(Token::Type::Minus);
            break;
        case '+':
            add_token(Token::Type::Plus);
            break;
        case ';':
            add_token(Token::Type::Semicolon);
            break;
        case '/':
            add_token(Token::Type::Slash);
            break;
        case '*':
            add_token(Token::Type::Star);
            break;
        case '!':
            add_token(match('=') ? Token::Type::BangEqual : Token::Type::Bang);
            break;
        case '=':
            add_token(match('=') ? Token::Type::EqualEqual : Token::Type::Equal);
            break;
        case '>':
            add_token(match('=') ? Token::Type::GreaterEqual : Token::Type::Greater);
            break;
        case '<':
            add_token(match('=') ? Token::Type::LessEqual : Token::Type::Less);
            break;
        default:
            if (is_identifier_first_char(ch)) {
                while (is_identifier_char(peek()))
                    ++m_end;
                add_token(Token::Type::Identifier);
            } else
                add_token(Token::Type::Invalid);
        }
    }
    return tokens;
}

}
