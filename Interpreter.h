#pragma once

#include "AST.h"
#include "Utils.h"
#include <cassert>
#include <vector>
#include <unordered_map>

namespace Lox {

class Object {
public:
    virtual ~Object() = default;

    virtual std::string_view type_name() const { return "Object"; }

    bool is_string() const { return type_name() == "String"; }
    bool is_number() const { return type_name() == "Number"; }
    bool is_bool() const { return type_name() == "Bool"; }
    bool is_niltype() const { return type_name() == "NilType"; }

    virtual std::string_view get_string() const { assert(0); }
    virtual double get_number() const { assert(0); }
    virtual bool get_bool() const { assert(0); }

    virtual bool __bool__() const { return true; }
    virtual bool __eq__(const Object&) const { return false; }

    virtual std::string __str__() const
    {
        return std::string("<").append(type_name()).append(">");
    }
};

class String : public Object {
public:
    String(std::string_view value) : m_value(value)
    {}
    String(std::string&& value) : m_value(std::move(value))
    {}

    std::string_view type_name() const override { return "String"; }
    std::string_view get_string() const override { return m_value; }
    bool __bool__() const override { return m_value.size() > 0; }

    bool __eq__(const Object& rhs) const override
    {
        return rhs.is_string() && rhs.get_string() == m_value;
    }

    std::string __str__() const override { return m_value; }

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
    bool __bool__() const override { return m_value != 0.0; }

    bool __eq__(const Object& rhs) const override
    {
        return rhs.is_number() && rhs.get_number() == m_value;
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
    bool __bool__() const override { return m_value; }

    bool __eq__(const Object& rhs) const override
    {
        return rhs.is_bool() && rhs.get_bool() == m_value;
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
    bool __bool__() const override { return false; }
    bool __eq__(const Object& rhs) const override { return rhs.is_niltype(); }
    std::string __str__() const override { return "nil"; }
};

inline std::shared_ptr<NilType> make_nil()
{
    return std::make_shared<NilType>();
}

class Interpreter {
public:
    explicit Interpreter(std::shared_ptr<Program> program)
        : m_program(program)
    {
        assert(program);
    }

    void interpret();

    using EnvType = std::unordered_map<std::string_view, std::shared_ptr<Object>>;
    const EnvType& env() const { return m_env; }
    void define_var(std::string_view name, std::shared_ptr<Object> value);
    std::shared_ptr<Object> get_var(std::string_view name) const;

    void error(std::string msg, std::string_view span);
    bool has_errors() const { return m_errors.size() > 0; }
    const std::vector<Error>& errors() const { return m_errors; }

private:
    std::shared_ptr<Program> m_program;
    std::vector<Error> m_errors;
    EnvType m_env;
};

}
