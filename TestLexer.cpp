#include "Lexer.h"
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cctype>
#include <cstdio>
#include <sys/wait.h>
namespace fs = std::filesystem;

using Lox::TokenType;

static void assert_tokens(std::string_view source,
                          std::vector<Lox::Token> tokens,
                          std::string_view error_span = {})
{
    Lox::Lexer lexer(source);
    auto output = lexer.lex();
    ASSERT_EQ(output.size(), tokens.size());
    for (std::size_t i = 0; i < output.size(); ++i) {
        const auto& lhs = output[i];
        const auto& rhs = tokens[i];
        EXPECT_EQ(lhs.type(), rhs.type());
        EXPECT_EQ(lhs.text(), rhs.text());
        EXPECT_EQ(lhs.value(), rhs.value());
    }

    if (error_span.empty())
        EXPECT_FALSE(lexer.has_errors());
    else {
        auto& errs = lexer.errors();
        ASSERT_EQ(errs.size(), 1);
        EXPECT_EQ(errs[0].source, source);
        EXPECT_EQ(errs[0].span, error_span);
    }
}

static void assert_token(std::string_view source, TokenType type,
                         Lox::Token::ValueType value = Lox::Token::DefaultValueType())
{
    assert_tokens(source, { { type, source, std::move(value) } });
}

static void assert_error(std::string_view source, std::string_view error_span = {})
{
    if (error_span.empty())
        error_span = source;
    assert_tokens(source, {}, error_span);
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

    assert_error(R"("foo\z")", "\\z");
    assert_error(R"("unterminated string)");
}

TEST(Lexer, Numbers)
{
    assert_token("9007199254740991", TokenType::Number, 9007199254740991.0);
    assert_token("3.14159265", TokenType::Number, 3.14159265);
    assert_token("4e9", TokenType::Number, 4e9);
    assert_token("7.843e-9", TokenType::Number, 7.843e-9);

    assert_error("1e999999");
}

TEST(Lexer, Comments)
{
    assert_tokens(R"(// commented line
        f(); // comment after code)", {
        { TokenType::Identifier, "f" },
        { TokenType::LeftParen, "(" },
        { TokenType::RightParen, ")" },
        { TokenType::Semicolon, ";" },
    });
}

TEST(Lexer, MultipleTokens)
{
    assert_tokens(R"(
        var foo = bar * 3.14;
        f(foo, "\tbaz");)", {
        { TokenType::Var, "var" },
        { TokenType::Identifier, "foo" },
        { TokenType::Equal, "=" },
        { TokenType::Identifier, "bar" },
        { TokenType::Star, "*" },
        { TokenType::Number, "3.14", 3.14 },
        { TokenType::Semicolon, ";" },
        { TokenType::Identifier, "f" },
        { TokenType::LeftParen, "(" },
        { TokenType::Identifier, "foo" },
        { TokenType::Comma, "," },
        { TokenType::String, R"("\tbaz")", "\tbaz" },
        { TokenType::RightParen, ")" },
        { TokenType::Semicolon, ";" },
    });
}

static void assert_lines(std::string_view source,
                         std::vector<std::size_t> line_limits)
{
    Lox::SourceMap smap(source);
    auto& limits = smap.line_limits();
    ASSERT_EQ(limits.size(), line_limits.size());
    for (std::size_t i = 0; i < limits.size(); ++i)
        EXPECT_EQ(limits[i], line_limits[i]);
}

TEST(SourceMap, LineLimits)
{
    assert_lines("", {});
    assert_lines("foo", { 3 });
    assert_lines("foo\n", { 4 });
    assert_lines(R"(
        var s = "multi
        line
        string";)", { 1, 24, 37, 53 });
}

TEST(SourceMap, Lines)
{
    Lox::SourceMap smap(R"(
        fn();
        var foo = "bar";)");
    EXPECT_EQ(smap.line(1), "");
    EXPECT_EQ(smap.line(2), "        fn();");
    EXPECT_EQ(smap.line(3), "        var foo = \"bar\";");
}

TEST(SourceMap, Ranges)
{
    std::string_view source = R"(
{
        var s = "multi
        line
        string
)";
    Lox::SourceMap smap(source);
    #define SPAN(start, len) source.substr(start, len)
    #define RANGE(...) (Lox::Range { __VA_ARGS__ })
    EXPECT_EQ(smap.span_to_range(SPAN(0, 54)), RANGE({ 1, 1 }, { 5, 16 })); // all
    EXPECT_EQ(smap.span_to_range(SPAN(1, 1)), RANGE({ 2, 1 }, { 2, 2 })); // {
    EXPECT_EQ(smap.span_to_range(SPAN(11, 3)), RANGE({ 3, 9 }, { 3, 12 })); // var
    EXPECT_EQ(smap.span_to_range(SPAN(19, 35)), RANGE({ 3, 17 }, { 5, 16 })); // literal
    #undef RANGE
    #undef SPAN
}

class Test : public testing::Test {
public:
    Test(const fs::path& source_path, const fs::path& output_path)
        : m_source_path(source_path)
        , m_output_path(output_path)
    {
        assert(!source_path.empty());
        assert(!output_path.empty());
    }

    void TestBody() override
    {
        errno = 0; // popen does not always set errno on error
        FILE* pipe = popen(std::string("</dev/null ./lox lex ")
            .append(m_source_path)
            .append(" ")
            .append(redirects())
            .c_str(), "r");
        ASSERT_NE(pipe, NULL);

        std::ostringstream real_out;
        for (;;) {
            char cbuf[4096];
            auto len = std::fread(cbuf, 1, sizeof(cbuf) - 1, pipe);
            if (len == 0) {
                int err = std::ferror(pipe);
                int ret = pclose(pipe);
                ASSERT_EQ(err, 0);
                ASSERT_TRUE(ret >= 0);
                ASSERT_TRUE(WIFEXITED(ret));
                ASSERT_EQ(WEXITSTATUS(ret), retcode());
                break; // eof
            }
            cbuf[len] = '\0';
            real_out << cbuf;
        }

        std::ostringstream mustbe_out;
        std::ifstream fin(m_output_path);
        ASSERT_TRUE(fin.is_open());
        while (mustbe_out << fin.rdbuf())
            ;
        ASSERT_FALSE(fin.bad());
        fin.close();

        EXPECT_EQ(mustbe_out.view(), real_out.view());
    }

private:
    virtual std::string redirects() const { return ""; }
    virtual int retcode() const { return 0; }

    fs::path m_source_path;
    fs::path m_output_path;
};

class PassingTest : public Test {
public:
    PassingTest(const fs::path& source_path, const fs::path& output_path)
        : Test(source_path, output_path)
    {}
};

class FailingTest : public Test {
public:
    FailingTest(const fs::path& source_path, const fs::path& output_path)
        : Test(source_path, output_path)
    {}

private:
    std::string redirects() const override
    {
        return "2>&1 >/dev/null";
    }

    int retcode() const override { return 1; }
};

[[noreturn]] static void error(const std::string& filename,
    const std::string& msg)
{
    std::cerr << "error: " << msg << ": " << filename << '\n';
    std::exit(1);
}

static bool is_alpha(char ch)
{
    return ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z');
}

static bool is_digit(char ch)
{
    return '0' <= ch && ch <= '9';
}

// GoogleTest requires that a test name must be a valid C++ identifier
// with no underscores. Otherwise such a test _silently passes_. Convert a
// hyphen- or undescore-delimited test file name into a camel-case test name.
static std::string test_path_to_name(const fs::path& path)
{
    std::string base = path.stem();
    if (base.empty())
        error(path, "test file name cannot be empty");
    if (!std::isalpha(base[0]))
        error(path, "test file name must start with a letter");

    // make camel-case
    base[0] = std::toupper(base[0]);
    for (std::size_t i = 0; i < base.size(); ++i) {
        if (base[i] == '-' || base[i] == '_') {
            if (i + 1 < base.size())
                base[i + 1] = std::toupper(base[i + 1]);
        } else if (!is_alpha(base[i]) && !is_digit(base[i]))
            error(path, "test file name may contain only letters, digits, hyphens and underscores");
    }

    // remove hyphens and underscores
    base.erase(std::remove(base.begin(), base.end(), '-'), base.end());
    base.erase(std::remove(base.begin(), base.end(), '_'), base.end());
    return base;
}

static void register_tests()
{
    fs::path tests_path("../tests");
    for (const auto& entry : fs::directory_iterator(tests_path / "lexer")) {
        if (entry.is_regular_file() && entry.path().extension() == ".lox") {
            auto source_path = entry.path();
            if (auto stdout_path = fs::path(source_path)
                    .replace_extension(".stdout");
                fs::is_regular_file(stdout_path)) {
                testing::RegisterTest("LexerPass",
                    test_path_to_name(source_path).c_str(),
                    nullptr, nullptr, __FILE__, __LINE__,
                    [=]() { return new PassingTest(source_path, stdout_path); });
            } else if (auto stderr_path = fs::path(source_path)
                    .replace_extension(".stderr");
                fs::is_regular_file(stderr_path)) {
                testing::RegisterTest("LexerFail",
                    test_path_to_name(source_path).c_str(),
                    nullptr, nullptr, __FILE__, __LINE__,
                    [=]() { return new FailingTest(source_path, stderr_path); });
            } else
                error(source_path, "missing corresponding .stdout/.stderr file");
        }
    }
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  register_tests();
  return RUN_ALL_TESTS();
}
