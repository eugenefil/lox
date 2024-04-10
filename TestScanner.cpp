#include "Scanner.h"

#include <gtest/gtest.h>

TEST(Scanner, EmptyInputReturnsNoTokens)
{
    ASSERT_TRUE(Lox::Scanner("").scan().empty());
}

TEST(Scanner, OneCharTokens)
{
    auto tokens = Lox::Scanner("(){},.-+;*").scan();
    using Type = Lox::Token::Type;
    std::vector<Type> types = {
        Type::LeftParen, Type::RightParen, Type::LeftBrace, Type::RightBrace,
        Type::Comma, Type::Dot, Type::Minus, Type::Plus,
        Type::Semicolon, Type::Star,
    };
    ASSERT_EQ(tokens.size(), types.size());
}
