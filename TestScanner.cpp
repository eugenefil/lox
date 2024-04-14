#include "Scanner.h"
#include <gtest/gtest.h>

using Type = Lox::Token::Type;

void assert_tokens(std::string_view input,
                   const std::vector<Lox::Token>& tokens)
{
    auto scanned = Lox::Scanner(input).scan();
    ASSERT_EQ(scanned.size(), tokens.size());
    for (std::size_t i = 0; i < tokens.size(); ++i)
        EXPECT_EQ(scanned[i], tokens[i]);
}

TEST(Scanner, EmptyInputReturnsNoTokens)
{
    assert_tokens("", {});
}

TEST(Scanner, OneCharTokens)
{
    assert_tokens("(){},.-+;/*", {
        { Type::LeftParen, "(" }, { Type::RightParen, ")" },
        { Type::LeftBrace, "{" }, { Type::RightBrace, "}" },
        { Type::Comma, "," }, { Type::Dot, "." },
        { Type::Minus, "-" }, { Type::Plus, "+" },
        { Type::Semicolon, ";" }, { Type::Slash, "/" }, { Type::Star, "*" },
    });
}

TEST(Scanner, SkipWhitespace)
{
    assert_tokens("\t(\n)\r\n{  }\t\t", {
        { Type::LeftParen, "(" }, { Type::RightParen, ")" },
        { Type::LeftBrace, "{" }, { Type::RightBrace, "}" },
    });
}

TEST(Scanner, OneTwoCharTokens)
{
    assert_tokens("!= ! == = >= > <= <", {
        { Type::BangEqual, "!=" }, { Type::Bang, "!" },
        { Type::EqualEqual, "==" }, { Type::Equal, "=" },
        { Type::GreaterEqual, ">=" }, { Type::Greater, ">" },
        { Type::LessEqual, "<=" }, { Type::Less, "<" },
    });
}

TEST(Scanner, Identifiers)
{
    assert_tokens("_ x0 foo_bar FOOBAR __foo3__BAR4__", {
        { Type::Identifier, "_" },
        { Type::Identifier, "x0" },
        { Type::Identifier, "foo_bar" },
        { Type::Identifier, "FOOBAR" },
        { Type::Identifier, "__foo3__BAR4__" },
    });
}

TEST(Scanner, Strings)
{
    assert_tokens(R"("" "hello world!" "\t\r\n\"\\")", {
        { Type::String, R"("")", "" },
        { Type::String, R"("hello world!")", "hello world!" },
        { Type::String, R"("\t\r\n\"\\")", "\t\r\n\"\\" },
    });
}
