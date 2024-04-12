#include "Scanner.h"

namespace Lox {

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.m_lexeme;
    return out;
}

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;
    auto append_one_char_token = [&](Token::Type type) {
        tokens.push_back(Token{type, m_source.substr(0, 1)});
        m_source.remove_prefix(1);
    };
    while (!m_source.empty()) {
        switch (m_source[0]) {
        case '(':
            append_one_char_token(Token::Type::LeftParen);
            break;
        case ')':
            append_one_char_token(Token::Type::RightParen);
            break;
        case '{':
            append_one_char_token(Token::Type::LeftBrace);
            break;
        case '}':
            append_one_char_token(Token::Type::RightBrace);
            break;
        case ',':
            append_one_char_token(Token::Type::Comma);
            break;
        case '.':
            append_one_char_token(Token::Type::Dot);
            break;
        case '-':
            append_one_char_token(Token::Type::Minus);
            break;
        case '+':
            append_one_char_token(Token::Type::Plus);
            break;
        case ';':
            append_one_char_token(Token::Type::Semicolon);
            break;
        case '/':
            append_one_char_token(Token::Type::Slash);
            break;
        case '*':
            append_one_char_token(Token::Type::Star);
            break;
        default:
            m_source.remove_prefix(1);
        }
    }
    return tokens;
}

}
