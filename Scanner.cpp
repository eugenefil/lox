#include "Scanner.h"
#include <cassert>

namespace Lox {

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.m_lexeme;
    return out;
}

bool Scanner::match(char next)
{
    if (m_end < m_source.size() && m_source[m_end] == next) {
        ++m_end;
        return true;
    }
    return false;
}

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;

    auto add_token = [&](Token::Type type) {
            assert(m_end > m_beg);
            tokens.push_back(Token { type, m_source.substr(m_beg, m_end - m_beg) });
            m_beg = m_end;
    };

    while (m_beg < m_source.size()) {
        auto ch = m_source[m_beg];
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
            add_token(Token::Type::Invalid);
        }
    }
    return tokens;
}

}
