#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct Token {
    enum class Type {
        LeftParen, RightParen, LeftBrace, RightBrace,
        Comma, Dot, Minus, Plus, Semicolon, Slash, Star,

        Bang, BangEqual,
        Equal, EqualEqual,
        Greater, GreaterEqual,
        Less, LessEqual,

        Identifier, String, Number,

        And, Class, Else, False, Fun, For, If, Nil, Or,
        Print, Return, Super, This, True, Var, While,
    };

    using Literal = std::variant<std::monostate, double, std::string_view>;

    Type m_type;
    std::string_view m_lexeme;
    Literal m_literal;

    Token(Type type, std::string_view lexeme)
        : m_type(type)
        , m_lexeme(lexeme)
    {}
};

class Scanner {
public:
    Scanner(std::string_view source);
    std::vector<Token> scan();

private:
    std::string_view m_source;
};

Scanner(std::string_view source) : m_source(source)
{}

std::vector<Token> Scanner::scan()
{
    std::vector<Token> tokens;
    while (!m_source.empty()) {
        switch (m_source[0]) {
        case '(':
            tokens.push_back(Token{Token::Type::LeftParen, m_source.substr(0, 1)});
            break;
        case ')':
            tokens.push_back(Token{Token::Type::RightParen, m_source.substr(0, 1)});
            break;
        case '{':
            tokens.push_back(Token{Token::Type::LeftBrace, m_source.substr(0, 1)});
            break;
        case '}':
            tokens.push_back(Token{Token::Type::RightBrace, m_source.substr(0, 1)});
            break;
        case ',':
            tokens.push_back(Token{Token::Type::Comma, m_source.substr(0, 1)});
            break;
        case '.':
            tokens.push_back(Token{Token::Type::Dot, m_source.substr(0, 1)});
            break;
        case '-':
            tokens.push_back(Token{Token::Type::Minus, m_source.substr(0, 1)});
            break;
        case '+':
            tokens.push_back(Token{Token::Type::Plus, m_source.substr(0, 1)});
            break;
        case ';':
            tokens.push_back(Token{Token::Type::Semicolon, m_source.substr(0, 1)});
            break;
        case '*':
            tokens.push_back(Token{Token::Type::Star, m_source.substr(0, 1)});
            break;
        default:
            m_source.remove_prefix(1);
        }
    }
    return tokens;
}

int main()
{
    return 0;
}
