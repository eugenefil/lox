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

char Scanner::peek() const
{
    if (m_source.size() < 2)
        return 0;
    return m_source[1];
}

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;

    auto add_token = [&](Token::Type type, size_t len) {
            tokens.push_back(Token { type, m_source.substr(0, len) });
            m_source.remove_prefix(len);
    };

    while (!m_source.empty()) {
        auto ch = m_source[0];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            m_source.remove_prefix(1);
        else if (ch == '!') {
            if (peek() == '=')
                add_token(Token::Type::BangEqual, 2);
            else
                add_token(Token::Type::Bang, 1);
        }
        else if (ch == '=') {
            if (peek() == '=')
                add_token(Token::Type::EqualEqual, 2);
            else
                add_token(Token::Type::Equal, 1);
        }
        else if (ch == '>') {
            if (peek() == '=')
                add_token(Token::Type::GreaterEqual, 2);
            else
                add_token(Token::Type::Greater, 1);
        }
        else if (ch == '<') {
            if (peek() == '=')
                add_token(Token::Type::LessEqual, 2);
            else
                add_token(Token::Type::Less, 1);
        }
        else if (auto type = s_one_char_tokens[ch];
            type != Token::Type::Invalid)
            add_token(type, 1);
        else
            add_token(Token::Type::Invalid, 1);
    }
    return tokens;
}

}
