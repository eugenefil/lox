#pragma once

#include "AST.h"
#include <cassert>
#include <vector>

namespace Lox {

class Object {
public:
    virtual ~Object() = default;

    virtual std::string_view type_name() const { return "Object"; }

    virtual std::string_view get_string() const { assert(0); }
    virtual double get_number() const { assert(0); }
    virtual bool get_bool() const { assert(0); }
};

class String : public Object {
public:
    String(std::string value) : m_value(value)
    {}

    std::string_view type_name() const override { return "String"; }
    std::string_view get_string() const override { return m_value; }

private:
    std::string m_value;
};

class Number : public Object {
public:
    Number(double value) : m_value(value)
    {}

    std::string_view type_name() const override { return "Number"; }
    double get_number() const override { return m_value; }

private:
    double m_value;
};

class Bool : public Object {
public:
    Bool(bool value) : m_value(value)
    {}

    std::string_view type_name() const override { return "Bool"; }
    bool get_bool() const override { return m_value; }

private:
    bool m_value;
};

class NilType : public Object {
public:
    std::string_view type_name() const override { return "NilType"; }
};

struct RuntimeError {
    std::string_view span;
    std::string_view msg;
};

class Interpreter {
public:
    explicit Interpreter(std::shared_ptr<Expr> ast) : m_ast(ast)
    {
        assert(ast);
    }

    std::shared_ptr<Object> interpret();

    bool has_errors() const { return m_errors.size() > 0; }

private:
    std::shared_ptr<Expr> m_ast;
    std::vector<RuntimeError> m_errors;
};

}
