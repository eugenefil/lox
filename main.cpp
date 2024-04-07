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

    using Literal = std::variant<std::monostate, double, std::string>;

    Type m_type;
    std::string_view m_lexeme;
    Literal m_literal;

    Token(Type type, std::string_view lexeme)
        : m_type(type)
        , m_lexeme(lexeme)
    {}

    friend std::ostream& operator<<(std::ostream& out, const Token& token);
};

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    out << token.m_lexeme;
    return out;
}

class Scanner {
public:
    Scanner(std::string_view source);
    std::vector<Token> scan();

private:
    std::string_view m_source;
};

Scanner::Scanner(std::string_view source) : m_source(source)
{}

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
        case '*':
            append_one_char_token(Token::Type::Star);
            break;
        default:
            m_source.remove_prefix(1);
        }
    }
    return tokens;
}

int main()
{
    std::string source = "(+){-}";
    for (const auto& tok : Scanner(source).scan())
        std::cout << tok << '\n';
    return 0;
}
