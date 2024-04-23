#pragma once

#include <variant>
#include <vector>
#include <string>

namespace Lox {

enum class TokenType {
    Invalid,

    // one-char tokens
    LeftParen, RightParen, LeftBrace, RightBrace,
    Comma, Dot, Minus, Plus, Semicolon, Star,

    // one- or two-char tokens
    Bang, BangEqual,
    Equal, EqualEqual,
    Greater, GreaterEqual,
    Less, LessEqual,
    Slash, Comment,

    // literals
    Identifier, String, Number,

    // keywords
    And, Class, Else, False, Fun, For, If, Nil, Or,
    Print, Return, Super, This, True, Var, While,

    Eof,
};

struct Span {
    std::size_t pos { 0 };
    std::size_t len { 0 };

    bool operator==(const Span&) const = default;
};

class Token {
public:
    using DefaultValueType = std::monostate;
    using ValueType = std::variant<DefaultValueType, bool, double, std::string>;

    Token(TokenType type, Span span, ValueType&& value = DefaultValueType())
        : m_type(type)
        , m_span(span)
        , m_value(std::move(value))
    {}

    bool operator==(const Token&) const = default;

    TokenType type() const { return m_type; }
    Span span() const { return m_span; }
    const ValueType& value() const { return m_value; }

private:
    TokenType m_type { TokenType::Invalid };
    Span m_span;
    ValueType m_value;
};

struct Error {
    Span span;
    std::string_view msg;
};

class Lexer {
public:
    Lexer(std::string_view input) : m_input(input)
    {}

    std::vector<Token> lex();
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    void advance() { ++m_end; }
    void consume() { m_start = m_end; }
    bool more() const { return m_end < m_input.size(); }
    char next() const { return m_input[m_end]; } // unsafe, guard with more()
    char peek() const { return more() ? next() : 0; }
    bool match(char next);

    std::string_view token_text() const;
    Span token_span() const;
    bool unescape(std::string&);
    void error(std::string_view msg, Span span = {});

    std::string_view m_input;
    std::size_t m_start { 0 };
    std::size_t m_end { 0 };
    std::vector<Error> m_errors;
};

}
