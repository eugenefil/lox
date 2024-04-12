#include "Scanner.h"

#include <gtest/gtest.h>

TEST(Scanner, EmptyInputReturnsNoTokens)
{
    ASSERT_TRUE(Lox::Scanner("").scan().empty());
}

TEST(Scanner, OneCharTokens)
{
    auto tokens = Lox::Scanner("(){},.-+;/*").scan();
    using Type = Lox::Token::Type;
    std::vector<Type> tok_types = {
        Type::LeftParen, Type::RightParen, Type::LeftBrace, Type::RightBrace,
        Type::Comma, Type::Dot, Type::Minus, Type::Plus,
        Type::Semicolon, Type::Slash, Type::Star,
    };
    ASSERT_EQ(tokens.size(), tok_types.size());

    for (std::size_t i = 0; i < tokens.size(); ++i)
        ASSERT_EQ(tokens[i].type(), tok_types[i]);
}

TEST(Scanner, SkipWhitespace)
{
    auto tokens = Lox::Scanner("\t(\n)\r\n{  }\t\t").scan();
    using Type = Lox::Token::Type;
    std::vector<Type> tok_types = {
        Type::LeftParen, Type::RightParen, Type::LeftBrace, Type::RightBrace,
    };
    ASSERT_EQ(tokens.size(), 4);

    for (std::size_t i = 0; i < tokens.size(); ++i)
        ASSERT_EQ(tokens[i].type(), tok_types[i]);
}
