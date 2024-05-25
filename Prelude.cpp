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
    for (auto& arg : args)
        std::cout << arg->__str__();
    std::cout << '\n';
    return make_nil();
}

void prelude(Interpreter& interp)
{
    interp.scope().define_var("print", std::make_shared<BuiltinFunction>(print, 1));
}

}
