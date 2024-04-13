#include "Scanner.h"
#include <gtest/gtest.h>

using Type = Lox::Token::Type;

void assert_token_types(std::string_view input, const std::vector<Type>& types)
{
    auto tokens = Lox::Scanner(input).scan();
    ASSERT_EQ(tokens.size(), types.size());
    for (std::size_t i = 0; i < tokens.size(); ++i)
        ASSERT_EQ(tokens[i].type(), types[i]);
}

TEST(Scanner, EmptyInputReturnsNoTokens)
{
    assert_token_types("", {});
}

TEST(Scanner, OneCharTokens)
{
    assert_token_types("(){},.-+;/*", {
        Type::LeftParen, Type::RightParen, Type::LeftBrace, Type::RightBrace,
        Type::Comma, Type::Dot, Type::Minus, Type::Plus,
        Type::Semicolon, Type::Slash, Type::Star,
    });
}

TEST(Scanner, SkipWhitespace)
{
    assert_token_types("\t(\n)\r\n{  }\t\t", {
        Type::LeftParen, Type::RightParen, Type::LeftBrace, Type::RightBrace,
    });
}

TEST(Scanner, OneTwoCharTokens)
{
    assert_token_types("!= ! == = >= > <= <", {
        Type::BangEqual, Type::Bang,
        Type::EqualEqual, Type::Equal,
        Type::GreaterEqual, Type::Greater,
        Type::LessEqual, Type::Less,
    });
}

TEST(Scanner, Identifiers)
{
    auto tokens = Lox::Scanner("_ x0 foo_bar FOOBAR __foo3__BAR4__").scan();
    ASSERT_EQ(tokens.size(), 5);

    for (const auto& tok : tokens)
        ASSERT_EQ(tok.type(), Type::Identifier);

    ASSERT_EQ(tokens[0].text(), "_");
    ASSERT_EQ(tokens[1].text(), "x0");
    ASSERT_EQ(tokens[2].text(), "foo_bar");
    ASSERT_EQ(tokens[3].text(), "FOOBAR");
    ASSERT_EQ(tokens[4].text(), "__foo3__BAR4__");
}
