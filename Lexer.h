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

// 1-based, char-oriented
struct Position {
    std::size_t line_num { 0 };
    std::size_t col_num { 0 };

    bool operator==(const Position&) const = default;
    bool valid() const { return line_num > 0 && col_num > 0; }
};

// [start, end) - exclusive
struct Range {
    Position start;
    Position end;

    bool operator==(const Range&) const = default;
    bool valid() const
    {
        return start.valid() && end.valid() && (
            end.line_num > start.line_num ||
            (end.line_num == start.line_num &&
                end.col_num > start.col_num)
        );
    }
};

class SourceMap {
public:
    SourceMap(std::string_view source);

    Range span_to_range(std::string_view span);
    std::string_view line(std::size_t line_num);
    const std::vector<std::size_t>& line_limits() const { return m_line_limits; }

private:
    std::vector<std::size_t> m_line_limits;
    std::string_view m_source;
};

class Token {
public:
    using DefaultValueType = std::monostate;
    using ValueType = std::variant<DefaultValueType, bool, double, std::string>;

    Token(TokenType type, std::string_view text, ValueType&& value = DefaultValueType())
        : m_type(type)
        , m_text(text)
        , m_value(std::move(value))
    {}

    bool operator==(const Token&) const = default;

    TokenType type() const { return m_type; }
    std::string_view text() const { return m_text; }
    const ValueType& value() const { return m_value; }

private:
    TokenType m_type { TokenType::Invalid };
    std::string_view m_text;
    ValueType m_value;
};

struct Error {
    std::string_view span;
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
    bool unescape(std::string&);
    void error(std::string_view msg, std::string_view span = {});

    std::string_view m_input;
    std::size_t m_start { 0 };
    std::size_t m_end { 0 };
    std::vector<Error> m_errors;
};

}
