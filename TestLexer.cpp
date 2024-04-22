#include "Lexer.h"
#include <gtest/gtest.h>

using Lox::TokenType;

void assert_tokens(std::string_view input,
                   const std::vector<Lox::Token>& tokens)
{
    auto output = Lox::Lexer(input).lex();
    ASSERT_EQ(output.size(), tokens.size());
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& lhs = output[i];
        const auto& rhs = tokens[i];
        EXPECT_EQ(lhs.type(), rhs.type());
        EXPECT_EQ(lhs.text(), rhs.text());
        EXPECT_EQ(lhs.value(), rhs.value());
    }
}

TEST(Lexer, EmptyInputReturnsNoTokens)
{
    assert_tokens("", {});
}

TEST(Lexer, OneCharTokens)
{
    assert_tokens("(){},.-+;/*@", {
        { TokenType::LeftParen, "(" },
        { TokenType::RightParen, ")" },
        { TokenType::LeftBrace, "{" },
        { TokenType::RightBrace, "}" },
        { TokenType::Comma, "," },
        { TokenType::Dot, "." },
        { TokenType::Minus, "-" },
        { TokenType::Plus, "+" },
        { TokenType::Semicolon, ";" },
        { TokenType::Slash, "/" },
        { TokenType::Star, "*" },
        { TokenType::Invalid, "@" },
    });
}

TEST(Lexer, SkipWhitespace)
{
    assert_tokens("\t(\n)\r\n{  }\t\t", {
        { TokenType::LeftParen, "(" },
        { TokenType::RightParen, ")" },
        { TokenType::LeftBrace, "{" },
        { TokenType::RightBrace, "}" },
    });
}

TEST(Lexer, OneTwoCharTokens)
{
    assert_tokens("!= ! == = >= > <= <", {
        { TokenType::BangEqual, "!=" },
        { TokenType::Bang, "!" },
        { TokenType::EqualEqual, "==" },
        { TokenType::Equal, "=" },
        { TokenType::GreaterEqual, ">=" },
        { TokenType::Greater, ">" },
        { TokenType::LessEqual, "<=" },
        { TokenType::Less, "<" },
    });
}

TEST(Lexer, Identifiers)
{
    assert_tokens("_ x0 foo_bar FOOBAR __foo3__BAR4__", {
        { TokenType::Identifier, "_" },
        { TokenType::Identifier, "x0" },
        { TokenType::Identifier, "foo_bar" },
        { TokenType::Identifier, "FOOBAR" },
        { TokenType::Identifier, "__foo3__BAR4__" },
    });
}

TEST(Lexer, Strings)
{
    assert_tokens(R"("" "hello world!" "\t\r\n\"\\" "foo\z" "multi
        line
        string" "newline \
escape"
        "unterminated string)", {
        { TokenType::String, R"("")", "" },
        { TokenType::String, R"("hello world!")", "hello world!" },
        { TokenType::String, R"("\t\r\n\"\\")", "\t\r\n\"\\" },
        { TokenType::Invalid, R"("foo\z")" },
        { TokenType::String, R"("multi
        line
        string")", "multi\n\
        line\n\
        string" },
        { TokenType::String, R"("newline \
escape")", "newline escape" },
        { TokenType::Invalid, R"("unterminated string)" },
    });
}

TEST(Lexer, Numbers)
{
    assert_tokens("9007199254740991 3.14159265 4e9 7.843e-9 1e999999", {
        { TokenType::Number, "9007199254740991", 9007199254740991.0 },
        { TokenType::Number, "3.14159265", 3.14159265 },
        { TokenType::Number, "4e9", 4e9},
        { TokenType::Number, "7.843e-9", 7.843e-9 },
        { TokenType::Invalid, "1e999999"},
    });
}

TEST(Lexer, Keywords)
{
    assert_tokens("and class else false fun for if nil or "
        "print return super this true var while", {
        { TokenType::And, "and" },
        { TokenType::Class, "class" },
        { TokenType::Else, "else" },
        { TokenType::False, "false", false },
        { TokenType::Fun, "fun" },
        { TokenType::For, "for" },
        { TokenType::If, "if" },
        { TokenType::Nil, "nil" },
        { TokenType::Or, "or" },
        { TokenType::Print, "print" },
        { TokenType::Return, "return" },
        { TokenType::Super, "super" },
        { TokenType::This, "this" },
        { TokenType::True, "true", true },
        { TokenType::Var, "var" },
        { TokenType::While, "while" },
    });
}

TEST(Lexer, Comments)
{
    assert_tokens(R"(// commented line
        f(); // comment after code)", {
        { TokenType::Comment, "// commented line" },
        { TokenType::Identifier, "f" },
        { TokenType::LeftParen, "(" },
        { TokenType::RightParen, ")" },
        { TokenType::Semicolon, ";" },
        { TokenType::Comment, "// comment after code" },
    });
}
