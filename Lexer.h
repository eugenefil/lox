#pragma once

#include "Utils.h"
#include <variant>
#include <vector>
#include <string>
#include <cassert>

namespace Lox {

#define FOR_EACH_TOKEN_TYPE \
    __TOKEN(LeftParen)      \
    __TOKEN(RightParen)     \
    __TOKEN(LeftBrace)      \
    __TOKEN(RightBrace)     \
    __TOKEN(Comma)          \
    __TOKEN(Dot)            \
    __TOKEN(Minus)          \
    __TOKEN(Plus)           \
    __TOKEN(Semicolon)      \
    __TOKEN(Star)           \
    __TOKEN(Bang)           \
    __TOKEN(BangEqual)      \
    __TOKEN(Equal)          \
    __TOKEN(EqualEqual)     \
    __TOKEN(Greater)        \
    __TOKEN(GreaterEqual)   \
    __TOKEN(Less)           \
    __TOKEN(LessEqual)      \
    __TOKEN(Slash)          \
    __TOKEN(Comment)        \
    __TOKEN(Identifier)     \
    __TOKEN(String)         \
    __TOKEN(Number)         \
    __TOKEN(And)            \
    __TOKEN(Assert)         \
    __TOKEN(Break)          \
    __TOKEN(Class)          \
    __TOKEN(Continue)       \
    __TOKEN(Else)           \
    __TOKEN(False)          \
    __TOKEN(Fn)             \
    __TOKEN(For)            \
    __TOKEN(If)             \
    __TOKEN(In)             \
    __TOKEN(Nil)            \
    __TOKEN(Or)             \
    __TOKEN(Percent)        \
    __TOKEN(Return)         \
    __TOKEN(Super)          \
    __TOKEN(This)           \
    __TOKEN(True)           \
    __TOKEN(Var)            \
    __TOKEN(While)          \
    __TOKEN(Eof)

enum class TokenType {
#define __TOKEN(x) x,
    FOR_EACH_TOKEN_TYPE
#undef __TOKEN
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

    std::string type_string() const
    {
        switch (m_type) {
#define __TOKEN(x)          \
        case TokenType::x:  \
            return #x;
        FOR_EACH_TOKEN_TYPE
#undef __TOKEN
        }
        assert(0);
    }
    std::string value_string() const;
    std::string dump() const;

private:
    const TokenType m_type;
    std::string_view m_text;
    ValueType m_value;
};

#undef FOR_EACH_TOKEN_TYPE

class Lexer {
public:
    Lexer(std::string_view source) : m_source(source)
    {
        assert(source.data());
    }

    std::vector<Token> lex();
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    void advance() { ++m_end; }
    void consume() { m_start = m_end; }
    bool more() const { return m_end < m_source.size(); }
    char next() const { return m_source[m_end]; } // unsafe, guard with more()
    char peek() const { return more() ? next() : 0; }
    bool match(char next);

    std::string_view token_text() const;
    bool unescape(std::string&);
    void error(std::string msg, std::string_view span = {});

    std::string_view m_source;
    std::size_t m_start { 0 };
    std::size_t m_end { 0 };
    std::vector<Error> m_errors;
};

}
