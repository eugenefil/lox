#include "Scanner.h"
#include <gtest/gtest.h>

using Type = Lox::Token::Type;

void assert_tokens(std::string_view input,
                   const std::vector<Lox::Token>& tokens)
{
    auto scanned = Lox::Scanner(input).scan();
    ASSERT_EQ(scanned.size(), tokens.size());
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& lhs = scanned[i];
        const auto& rhs = tokens[i];
        EXPECT_EQ(lhs.type(), rhs.type());
        EXPECT_EQ(lhs.text(), rhs.text());
        EXPECT_EQ(lhs.value(), rhs.value());
    }
}

TEST(Scanner, EmptyInputReturnsNoTokens)
{
    assert_tokens("", {});
}

TEST(Scanner, OneCharTokens)
{
    assert_tokens("(){},.-+;/*@", {
        { Type::LeftParen, "(" }, { Type::RightParen, ")" },
        { Type::LeftBrace, "{" }, { Type::RightBrace, "}" },
        { Type::Comma, "," }, { Type::Dot, "." },
        { Type::Minus, "-" }, { Type::Plus, "+" },
        { Type::Semicolon, ";" }, { Type::Slash, "/" }, { Type::Star, "*" },
        { Type::Invalid, "@" },
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
    assert_tokens(R"("" "hello world!" "\t\r\n\"\\" "foo\z" "multi
        line
        string" "newline \
escape"
        "unterminated string)", {
        { Type::String, R"("")", "" },
        { Type::String, R"("hello world!")", "hello world!" },
        { Type::String, R"("\t\r\n\"\\")", "\t\r\n\"\\" },
        { Type::Invalid, R"("foo\z")" },
        { Type::String, R"("multi
        line
        string")", "multi\n\
        line\n\
        string" },
        { Type::String, R"("newline \
escape")", "newline escape" },
        { Type::Invalid, R"("unterminated string)" },
    });
}

TEST(Scanner, Numbers)
{
    assert_tokens("9007199254740991 3.14159265 4e9 7.843e-9 1e999999", {
        { Type::Number, "9007199254740991", 9007199254740991.0 },
        { Type::Number, "3.14159265", 3.14159265 },
        { Type::Number, "4e9", 4e9},
        { Type::Number, "7.843e-9", 7.843e-9 },
        { Type::Invalid, "1e999999"},
    });
}

TEST(Scanner, ReservedKeywords)
{
    assert_tokens("And Class Else False Fun For If Nil Or "
        "Print Return Super This True Var While", {
        { Type::And, "And" },
        { Type::Class, "Class" },
        { Type::Else, "Else" },
        { Type::False, "False" },
        { Type::Fun, "Fun" },
        { Type::For, "For" },
        { Type::If, "If" },
        { Type::Nil, "Nil" },
        { Type::Or, "Or" },
        { Type::Print, "Print" },
        { Type::Return, "Return" },
        { Type::Super, "Super" },
        { Type::This, "This" },
        { Type::True, "True" },
        { Type::Var, "Var" },
        { Type::While, "While" },
    });
}
