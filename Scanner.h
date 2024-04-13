#pragma once

#include <variant>
#include <vector>
#include <string>

namespace Lox {

struct Token {
    enum class Type {
        Invalid,

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

    Token(Type type, std::string_view text)
        : m_type(type)
        , m_text(text)
    {}

    Type type() const { return m_type; }
    std::string_view text() const { return m_text; }

    friend std::ostream& operator<<(std::ostream& out, const Token& token);

private:
    Type m_type { Type::Invalid };
    std::string_view m_text;
    Literal m_literal;
};

class Scanner {
public:
    Scanner(std::string_view source) : m_input(source)
    {}

    std::vector<Token> scan();

private:
    char peek() const;
    bool match(char next);

    std::string_view m_input;
    std::size_t m_beg { 0 };
    std::size_t m_end { 0 };
};

}
