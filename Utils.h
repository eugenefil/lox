#pragma once

#include <vector>
#include <string>

namespace Lox {

// 1-based, char-oriented
struct Position {
    std::size_t line_num { 0 };
    std::size_t col_num { 0 };

    bool operator==(const Position&) const = default;
    bool valid() const { return line_num > 0 && col_num > 0; }
};

// [start, end) - exclusive
struct Range {
    Position start;
    Position end;

    bool operator==(const Range&) const = default;
    bool valid() const
    {
        return start.valid() && end.valid() && (
            end.line_num > start.line_num ||
            (end.line_num == start.line_num &&
                end.col_num > start.col_num)
        );
    }
};

class SourceMap {
public:
    SourceMap(std::string_view source);

    Range span_to_range(std::string_view span);
    std::string_view line(std::size_t line_num);
    const std::vector<std::size_t>& line_limits() const { return m_line_limits; }

private:
    std::vector<std::size_t> m_line_limits;
    std::string_view m_source;
};

struct Error {
    std::string msg;
    std::string_view span;
};

std::string escape(std::string s);

}
