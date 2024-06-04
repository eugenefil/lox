#include "Utils.h"
#include <gtest/gtest.h>

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
