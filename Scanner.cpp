#include "Scanner.h"
#include <array>

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

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;
    while (!m_source.empty()) {
        auto ch = m_source[0];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            m_source.remove_prefix(1);
        else if (auto type = s_one_char_tokens[ch];
            type != Token::Type::Invalid) {
            tokens.push_back(Token{type, m_source.substr(0, 1)});
            m_source.remove_prefix(1);
        }
    }
    return tokens;
}

}
