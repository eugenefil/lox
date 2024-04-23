#include "Lexer.h"
#include <gtest/gtest.h>

using Lox::TokenType;

void assert_tokens(std::string_view input,
                   std::vector<Lox::Token> tokens,
                   std::vector<Lox::Span> error_spans = {})
{
    Lox::Lexer lexer(input);
    auto output = lexer.lex();
    ASSERT_EQ(output.size(), tokens.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        const auto& lhs = output[i];
        const auto& rhs = tokens[i];
        EXPECT_EQ(lhs.type(), rhs.type());
        EXPECT_EQ(lhs.span(), rhs.span());
        EXPECT_EQ(lhs.value(), rhs.value());
    }

    auto& errors = lexer.errors();
    ASSERT_EQ(errors.size(), error_spans.size());
    for (std::size_t i = 0; i < errors.size(); ++i)
        EXPECT_EQ(errors[i].span, error_spans[i]);
}

void assert_token(std::string_view input, TokenType type,
                  Lox::Token::ValueType value = Lox::Token::DefaultValueType())
{
    assert_tokens(input, { { type, { 0, input.size() }, std::move(value) } });
}

void assert_invalid_token(std::string_view input, Lox::Span error_span = {})
{
    if (error_span == Lox::Span {})
        error_span = { 0, input.size() };
    assert_tokens(input, { { TokenType::Invalid, { 0, input.size() } } },
                  { error_span });
}

TEST(Lexer, EmptyInputReturnsNoTokens)
{
    assert_tokens("", {});
}

TEST(Lexer, OneCharTokens)
{
    assert_token("(", TokenType::LeftParen);
    assert_token(")", TokenType::RightParen);
    assert_token("{", TokenType::LeftBrace);
    assert_token("}", TokenType::RightBrace);
    assert_token(",", TokenType::Comma);
    assert_token(".", TokenType::Dot);
    assert_token("-", TokenType::Minus);
    assert_token("+", TokenType::Plus);
    assert_token(";", TokenType::Semicolon);
    assert_token("*", TokenType::Star);
    assert_token("/", TokenType::Slash);

    assert_invalid_token("@");
}

TEST(Lexer, SkipWhitespace)
{
    assert_tokens("\t(\n)\r\n{  }\t\t", {
        { TokenType::LeftParen, { 1, 1 } },
        { TokenType::RightParen, { 3, 1 } },
        { TokenType::LeftBrace, { 6, 1 } },
        { TokenType::RightBrace, { 9, 1 } },
    });
}

TEST(Lexer, OneTwoCharTokens)
{
    assert_token("!", TokenType::Bang);
    assert_token("!=", TokenType::BangEqual);
    assert_token("=", TokenType::Equal);
    assert_token("==", TokenType::EqualEqual);
    assert_token(">", TokenType::Greater);
    assert_token(">=", TokenType::GreaterEqual);
    assert_token("<", TokenType::Less);
    assert_token("<=", TokenType::LessEqual);
}

TEST(Lexer, Identifiers)
{
    assert_token("_", TokenType::Identifier);
    assert_token("x0", TokenType::Identifier);
    assert_token("foo_bar", TokenType::Identifier);
    assert_token("FOOBAR", TokenType::Identifier);
    assert_token("__foo3__BAR4__", TokenType::Identifier);
}

TEST(Lexer, Strings)
{
    assert_token(R"("")", TokenType::String, "");
    assert_token(R"("hello world!")", TokenType::String, "hello world!");
    assert_token(R"("\t\r\n\"\\")", TokenType::String, "\t\r\n\"\\");
    assert_token(R"("multi
        line
        string")", TokenType::String, "multi\n\
        line\n\
        string");
    assert_token(R"("newline \
escape")", TokenType::String, "newline escape");

    assert_invalid_token(R"("foo\z")", { 4, 2 });
    assert_invalid_token(R"("unterminated string)");
}

TEST(Lexer, Numbers)
{
    assert_token("9007199254740991", TokenType::Number, 9007199254740991.0);
    assert_token("3.14159265", TokenType::Number, 3.14159265);
    assert_token("4e9", TokenType::Number, 4e9);
    assert_token("7.843e-9", TokenType::Number, 7.843e-9);

    assert_invalid_token("1e999999");
}

TEST(Lexer, Keywords)
{
    assert_token("and", TokenType::And);
    assert_token("class", TokenType::Class);
    assert_token("else", TokenType::Else);
    assert_token("false", TokenType::False, false);
    assert_token("fun", TokenType::Fun);
    assert_token("for", TokenType::For);
    assert_token("if", TokenType::If);
    assert_token("nil", TokenType::Nil);
    assert_token("or", TokenType::Or);
    assert_token("print", TokenType::Print);
    assert_token("return", TokenType::Return);
    assert_token("super", TokenType::Super);
    assert_token("this", TokenType::This);
    assert_token("true", TokenType::True, true);
    assert_token("var", TokenType::Var);
    assert_token("while", TokenType::While);
}

TEST(Lexer, Comments)
{
    assert_tokens(R"(// commented line
        f(); // comment after code)", {
        { TokenType::Comment, { 0, 17 } },
        { TokenType::Identifier, { 26, 1 } },
        { TokenType::LeftParen, { 27, 1 } },
        { TokenType::RightParen, { 28, 1 } },
        { TokenType::Semicolon, { 29, 1 } },
        { TokenType::Comment, { 31, 21 } },
    });
}
