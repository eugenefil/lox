#include "Scanner.h"
#include <array>
#include <cassert>

namespace Lox {

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.m_lexeme;
    return out;
}

static consteval std::array<Token::Type, 256> make_one_char_token_array()
{
    std::array<Token::Type, 256> array = { Token::Type::Invalid };
    array['('] = Token::Type::LeftParen;
    array[')'] = Token::Type::RightParen;
    array['{'] = Token::Type::LeftBrace;
    array['}'] = Token::Type::RightBrace;
    array[','] = Token::Type::Comma;
    array['.'] = Token::Type::Dot;
    array['-'] = Token::Type::Minus;
    array['+'] = Token::Type::Plus;
    array[';'] = Token::Type::Semicolon;
    array['/'] = Token::Type::Slash;
    array['*'] = Token::Type::Star;
    return array;
}

static constexpr auto s_one_char_tokens = make_one_char_token_array();

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
            if (auto type = s_one_char_tokens[ch]; type != Token::Type::Invalid)
                add_token(type);
            else
                add_token(Token::Type::Invalid);
        }
    }
    return tokens;
}

}
