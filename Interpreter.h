#pragma once

#include "AST.h"
#include "Utils.h"
#include <cassert>
#include <vector>
#include <unordered_map>
#include <csignal>

namespace Lox {

class Interpreter;
class Iterator;

class Object {
public:
    virtual ~Object() = default;

    virtual std::string_view type_name() const { return "Object"; }

    bool is_string() const { return type_name() == "String"; }
    bool is_number() const { return type_name() == "Number"; }
    bool is_bool() const { return type_name() == "Bool"; }
    bool is_niltype() const { return type_name() == "NilType"; }
    bool is_function() const { return type_name() == "Function"; }

    virtual std::string_view get_string() const { assert(0); }
    virtual double get_number() const { assert(0); }
    virtual bool get_bool() const { assert(0); }

    virtual bool __eq__(const Object&) const { return false; }
    virtual std::string __str__() const
    {
        return std::string("<").append(type_name()).append(">");
    }

    virtual std::shared_ptr<Object> __call__(
        const std::vector<std::shared_ptr<Object>>&, Interpreter&) { assert(0); }
    virtual std::size_t arity() const { assert(0); }

    virtual bool is_iterable() const { return false; }
    virtual std::shared_ptr<Iterator> __iter__() const { assert(0); }
};

class String : public Object
    , public std::enable_shared_from_this<String> {
public:
    String(std::string_view value) : m_value(value)
    {}
    String(std::string&& value) : m_value(std::move(value))
    {}

    std::string_view type_name() const override { return "String"; }
    std::string_view get_string() const override { return m_value; }

    std::size_t size() const { return m_value.size(); }
    std::string get_char(std::size_t pos) const
    {
        assert(pos < m_value.size());
        return std::string(1, m_value[pos]);
    }

    bool __eq__(const Object& rhs) const override
    {
        assert(rhs.is_string());
        return rhs.get_string() == m_value;
    }

    std::string __str__() const override { return m_value; }

    bool is_iterable() const override { return true; }
    std::shared_ptr<Iterator> __iter__() const override;

private:
    std::string m_value;
};

inline std::shared_ptr<String> make_string(std::string_view val)
{
    return std::make_shared<String>(val);
}

class Number : public Object {
public:
    Number(double value) : m_value(value)
    {}

    std::string_view type_name() const override { return "Number"; }
    double get_number() const override { return m_value; }

    bool __eq__(const Object& rhs) const override
    {
        assert(rhs.is_number());
        return rhs.get_number() == m_value;
    }

    std::string __str__() const override;

private:
    double m_value;
};

inline std::shared_ptr<Number> make_number(double val)
{
    return std::make_shared<Number>(val);
}

class Bool : public Object {
public:
    Bool(bool value) : m_value(value)
    {}

    std::string_view type_name() const override { return "Bool"; }
    bool get_bool() const override { return m_value; }

    bool __eq__(const Object& rhs) const override
    {
        assert(rhs.is_bool());
        return rhs.get_bool() == m_value;
    }

    std::string __str__() const override { return m_value ? "true" : "false"; }

private:
    bool m_value;
};

inline std::shared_ptr<Bool> make_bool(bool val)
{
    return std::make_shared<Bool>(val);
}

class NilType : public Object {
public:
    std::string_view type_name() const override { return "NilType"; }

    bool __eq__(const Object& rhs) const override
    {
        assert(rhs.is_niltype());
        return true;
    }

    std::string __str__() const override { return "nil"; }
};

inline std::shared_ptr<NilType> make_nil()
{
    return std::make_shared<NilType>();
}

class Function : public Object {
public:
    explicit Function(std::shared_ptr<const FunctionDeclaration> decl,
                      std::string_view program_source)
        : m_decl(decl)
        , m_program_source(program_source)
    {
        assert(decl);
        assert(!program_source.empty());
    }

    std::string_view type_name() const override { return "Function"; }
    std::shared_ptr<const FunctionDeclaration> decl() const { return m_decl; }

    std::shared_ptr<Object> __call__(
        const std::vector<std::shared_ptr<Object>>&, Interpreter&) override;
    std::size_t arity() const override { return m_decl->params().size(); }

private:
    std::shared_ptr<const FunctionDeclaration> m_decl;
    // source of the program where function was defined, for error reporting
    std::string_view m_program_source;
};

class Scope {
public:
    using MapType = std::unordered_map<std::string_view, std::shared_ptr<Object>>;

    Scope() = default;
    explicit Scope(std::shared_ptr<Scope> parent) : m_parent(parent)
    {
        assert(parent);
    }

    bool is_top() const { return m_parent == nullptr; }
    void define_var(std::string_view name, std::shared_ptr<Object> value);
    std::shared_ptr<Object> get_var(std::string_view name) const;
    bool set_var(std::string_view name, std::shared_ptr<Object> value);

    const MapType& vars() const { return m_vars; }

private:
    std::shared_ptr<Scope> m_parent;
    MapType m_vars;
};

class Interpreter {
public:
    Interpreter() : m_scope(std::make_shared<Scope>())
    {}

    void interpret(std::shared_ptr<Program> program);

    Scope& scope() { return *m_scope; }
    TemporaryChange<std::shared_ptr<Scope>> push_scope()
    {
        return { m_scope, std::make_shared<Scope>(m_scope) };
    }

    std::string_view source() const { return m_source; }
    TemporaryChange<std::string_view> push_source(std::string_view source)
    {
        return { m_source, source };
    }

    void error(std::string msg, std::string_view span);
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

    bool is_repl_mode() const { return m_repl_mode; }
    void repl_mode(bool on) { m_repl_mode = on; }

    bool is_break() const { return m_break; }
    void set_break(bool on)
    {
        assert(m_break != on);
        m_break = on;
    }

    bool is_continue() const { return m_continue; }
    void set_continue(bool on)
    {
        assert(m_continue != on);
        m_continue = on;
    }

    bool is_return() const { return m_return_value != nullptr; }
    void set_return_value(const std::shared_ptr<Object>& value)
    {
        assert(value);
        assert(!m_return_value);
        m_return_value = value;
    }
    std::shared_ptr<Object>&& pop_return_value()
    {
        assert(m_return_value);
        return std::move(m_return_value);
    }

    bool check_interrupt();

private:
    std::vector<Error> m_errors;
    std::shared_ptr<Scope> m_scope;
    bool m_repl_mode { false };
    bool m_break { false };
    bool m_continue { false };
    std::shared_ptr<Object> m_return_value;
    std::string_view m_source;
};

extern volatile std::sig_atomic_t g_interrupt;

}
