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

    using DefaultValueType = std::monostate;
    using ValueType = std::variant<DefaultValueType, double, std::string>;

    Token(Type type, std::string_view text, ValueType&& value = DefaultValueType())
        : m_type(type)
        , m_text(text)
        , m_value(std::move(value))
    {}

    bool operator==(const Token&) const = default;

    Type type() const { return m_type; }
    std::string_view text() const { return m_text; }
    const ValueType& value() const { return m_value; }

    friend std::ostream& operator<<(std::ostream& out, const Token& token);

private:
    Type m_type { Type::Invalid };
    std::string_view m_text;
    ValueType m_value;
};

class Scanner {
public:
    Scanner(std::string_view source) : m_input(source)
    {}

    std::vector<Token> scan();

private:
    void advance() { ++m_end; }
    bool more() const { return m_end < m_input.size(); }
    char next() const { return m_input[m_end]; } // unsafe, guard with more()
    char peek() const { return more() ? next() : 0; }
    bool match(char next);

    std::string_view token_text() const;
    bool unescape(std::string&);
    void error(std::string_view) {};

    std::string_view m_input;
    std::size_t m_beg { 0 };
    std::size_t m_end { 0 };
};

}
