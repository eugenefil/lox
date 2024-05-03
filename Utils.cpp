#include "Utils.h"
#include <cassert>

namespace Lox {

SourceMap::SourceMap(std::string_view source) : m_source(source)
{
    std::size_t start = 0;
    std::size_t end = 0;
    for (; end < m_source.size(); ++end) {
        if (m_source[end] == '\n') {
            m_line_limits.push_back(end + 1);
            start = end + 1;
        }
    }
    if (end > start)
        m_line_limits.push_back(end);
}

Range SourceMap::span_to_range(std::string_view span)
{
    assert(span.data());
    assert(span.size() > 0);
    assert(span.data() >= m_source.data());
    assert(span.data() + span.size() <= m_source.data() + m_source.size());
    std::size_t start = span.data() - m_source.data();
    assert(start < m_source.size());
    std::size_t end = start + span.size() - 1;
    assert(end < m_source.size());

    auto find_line_num = [this](std::size_t pos) -> std::size_t {
        for (std::size_t i = 0; i < m_line_limits.size(); ++i) {
            if (pos < m_line_limits[i])
                return i + 1;
        }
        return 0;
    };

    auto find_col_num = [this](std::size_t pos, std::size_t line_num) {
        auto line_start = line_num > 1 ? m_line_limits[line_num - 2] : 0;
        assert(pos >= line_start);
        return pos - line_start + 1;
    };

    auto start_line_num = find_line_num(start);
    assert(start_line_num > 0);
    auto start_col_num = find_col_num(start, start_line_num);
    assert(start_col_num > 0);
    auto end_line_num = find_line_num(end);
    assert(end_line_num > 0);
    auto end_col_num = find_col_num(end, end_line_num);
    assert(end_col_num > 0);

    Range range = { { start_line_num, start_col_num },
                    { end_line_num, end_col_num + 1 } }; // exclusive
    assert(range.valid());
    return range;
}

std::string_view SourceMap::line(std::size_t line_num)
{
    assert(line_num > 0);
    assert(line_num <= m_line_limits.size());
    --line_num;
    auto start = line_num > 0 ? m_line_limits[line_num - 1] : 0;
    auto end = m_line_limits[line_num];
    assert(end > start);
    auto line = m_source.substr(start, end - start);
    if (line.ends_with('\n'))
        line.remove_suffix(1);
    return line;
}

}
