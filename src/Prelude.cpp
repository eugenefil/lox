#include "Interpreter.h"
#include <iostream>

namespace Lox {

using ArgsVector = std::vector<std::shared_ptr<Object>>;
using BuiltinFunctionPtr =
    std::shared_ptr<Object> (*)(const ArgsVector&, Interpreter&);

class BuiltinFunction : public Callable {
public:
    BuiltinFunction(BuiltinFunctionPtr func, std::size_t arity)
        : m_func(func)
        , m_arity(arity)
    {
        assert(func);
    }

    std::string_view type_name() const override { return "BuiltinFunction"; }

    std::shared_ptr<Object> __call__(const ArgsVector& args,
                                     Interpreter& interp) override
    {
        return m_func(args, interp);
    }

    std::size_t arity() const override { return m_arity; }

private:
    const BuiltinFunctionPtr m_func;
    std::size_t m_arity { 0 };
};

static std::shared_ptr<Object> print(const ArgsVector& args, Interpreter&)
{
    std::cout << args[0]->__str__();
    std::cout << '\n';
    return make_nil();
}

static std::shared_ptr<Object> input(const ArgsVector& args, Interpreter&)
{
    std::cout << args[0]->__str__();
    std::string line;
    if (std::getline(std::cin, line))
        return make_string(std::move(line));
    return {};
}

void prelude(Interpreter& interp)
{
    interp.define_var("print", std::make_shared<BuiltinFunction>(print, 1));
    interp.define_var("input", std::make_shared<BuiltinFunction>(input, 1));
}

}
