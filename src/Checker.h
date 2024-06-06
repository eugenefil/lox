#pragma once

#include "AST.h"
#include "Utils.h"
#include <list>
#include <unordered_map>

namespace Lox {

class Checker {
public:
    void check(const std::shared_ptr<Program>& program);

    void error(std::string msg, std::string_view span);
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

    void push_scope();
    void pop_scope();
    void declare(std::string_view name);
    std::optional<std::size_t> hops_to_name(std::string_view name);

private:
    std::vector<Error> m_errors;
    std::list<std::unordered_map<std::string_view, bool>> m_scope_stack;
    std::string_view m_source;
};

}
