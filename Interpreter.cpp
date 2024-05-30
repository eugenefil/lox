#include "Interpreter.h"
#include <format>
#include <iostream>
#include <cmath>

namespace Lox {

class Iterator {
public:
    virtual bool done() const = 0;
    virtual std::shared_ptr<Object> next() = 0;
};

class StringIterator : public Iterator {
public:
    explicit StringIterator(std::shared_ptr<const String> str) : m_str(str)
    {
        assert(str);
    }

    bool done() const override { return m_pos >= m_str->size(); }

    std::shared_ptr<Object> next() override
    {
        assert(!done());
        return std::make_shared<String>(m_str->get_char(m_pos++));
    }

private:
    std::shared_ptr<const String> m_str;
    std::size_t m_pos { 0 };
};

std::shared_ptr<Iterator> String::__iter__() const
{
    return std::make_shared<StringIterator>(shared_from_this());
}

std::string Number::__str__() const
{
    return number_to_string(m_value);
}

static bool execute_statements(const std::vector<std::shared_ptr<Stmt>>& stmts,
                               Interpreter& interp)
{
    for (auto& stmt : stmts) {
        if (!stmt->execute(interp))
            return false;
    }
    return true;
}

std::shared_ptr<Object> Function::__call__(
    const std::vector<std::shared_ptr<Object>>& args,
    Interpreter& interp)
{
    // in a repl, function could be defined by some previous code chunk,
    // that is different from the one currently executed; temporarily set
    // that chunk's program source as interpreter's current source, so that
    // if error happens, error's source field points to the correct source
    // TemporaryChange object will restore original source on destruction
    auto source_change = interp.push_source(m_program_source);

    assert(!interp.is_return());

    auto scope_change = interp.new_scope(m_parent_scope);
    auto& params = m_func->params();
    assert(params.size() == args.size());
    for (std::size_t i = 0; i < args.size(); ++i)
        interp.scope().define_var(params[i]->name(), args[i]);
    auto res = execute_statements(m_func->block().statements(), interp);

    if (!res) {
        if (interp.is_return())
            return interp.pop_return_value();
        return {};
    }
    return make_nil(); // implicit return
}

std::shared_ptr<Object> StringLiteral::eval(Interpreter&) const
{
    return make_string(m_value);
}

std::shared_ptr<Object> NumberLiteral::eval(Interpreter&) const
{
    return make_number(m_value);
}

std::shared_ptr<Object> Identifier::eval(Interpreter& interp) const
{
    if (auto val = interp.scope().get_var(m_name))
        return val;
    interp.error(std::format("identifier '{}' is not defined", m_name), m_text);
    return {};
}

std::shared_ptr<Object> BoolLiteral::eval(Interpreter&) const
{
    return make_bool(m_value);
}

std::shared_ptr<Object> NilLiteral::eval(Interpreter&) const
{
    return make_nil();
}

std::shared_ptr<Object> UnaryExpr::eval(Interpreter& interp) const
{
    auto obj = m_expr->eval(interp);
    if (!obj)
        return {};

    switch (m_op) {
    case UnaryOp::Minus:
        if (!obj->is_number()) {
            interp.error(std::format("cannot apply unary operator '-' to type '{}'",
                obj->type_name()), m_text);
            return {};
        }
        return make_number(-obj->get_number());

    case UnaryOp::Not:
        if (!obj->is_bool()) {
            interp.error(std::format("cannot apply unary operator '!' to type '{}'",
                obj->type_name()), m_text);
            return {};
        }
        return make_bool(!obj->get_bool());
    }
    assert(0);
}

std::shared_ptr<Object> GroupExpr::eval(Interpreter& interp) const
{
    return m_expr->eval(interp);
}

std::shared_ptr<Object> BinaryExpr::eval(Interpreter& interp) const
{
    auto left = m_left->eval(interp);
    if (!left)
        return {};
    auto right = m_right->eval(interp);
    if (!right)
        return {};

    switch (m_op) {
    case BinaryOp::Divide:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() / right->get_number());
        interp.error(std::format("cannot divide '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Multiply:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() * right->get_number());
        interp.error(std::format("cannot multiply '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Modulo:
        if (left->is_number() && right->is_number())
            return make_number(std::fmod(left->get_number(), right->get_number()));
        interp.error(std::format("cannot divide '{}' by '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Add:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() + right->get_number());
        else if (left->is_string() && right->is_string())
            return make_string(std::string(left->get_string())
                .append(right->get_string()));
        interp.error(std::format("cannot add '{}' to '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Subtract:
        if (left->is_number() && right->is_number())
            return make_number(left->get_number() - right->get_number());
        interp.error(std::format("cannot subtract '{}' from '{}'",
            right->type_name(), left->type_name()), m_text);
        return {};

    case BinaryOp::Equal:
        if (left->type_name() == right->type_name())
            return make_bool(left->__eq__(*right));
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::NotEqual:
        if (left->type_name() == right->type_name())
            return make_bool(!left->__eq__(*right));
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Less:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() < right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() < right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::LessOrEqual:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() <= right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() <= right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::Greater:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() > right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() > right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};

    case BinaryOp::GreaterOrEqual:
        if (left->is_number() && right->is_number())
            return make_bool(left->get_number() >= right->get_number());
        else if (left->is_string() && right->is_string())
            return make_bool(left->get_string() >= right->get_string());
        interp.error(std::format("cannot compare '{}' with '{}'",
            left->type_name(), right->type_name()), m_text);
        return {};
    };
    assert(0);
}

std::shared_ptr<Object> LogicalExpr::eval(Interpreter& interp) const
{
    auto left = m_left->eval(interp);
    if (!left)
        return {};
    if (!left->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", left->type_name()),
            m_left->text());
        return {};
    }

    switch (m_op) {
    case LogicalOp::And:
        if (!left->get_bool())
            return make_bool(false);
        break;
    case LogicalOp::Or:
        if (left->get_bool())
            return make_bool(true);
        break;
    default:
        assert(0);
    }

    auto right = m_right->eval(interp);
    if (!right)
        return {};
    if (!right->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", right->type_name()),
            m_right->text());
        return {};
    }
    return make_bool(right->get_bool());
}

std::shared_ptr<Object> CallExpr::eval(Interpreter& interp) const
{
    auto callee = m_callee->eval(interp);
    if (!callee)
        return {};
    if (!callee->is_callable()) {
        interp.error(std::format("'{}' object is not callable",
            callee->type_name()), m_callee->text());
        return {};
    }
    auto& callable = static_cast<Callable&>(*callee);

    // if arity were to be checked before eval'ing the args, then an arity
    // error message with the invalid arguments supplied would look like the
    // interpreter has validated the arguments and went on to check arity:
    //
    // >>> fn f() {}
    // >>> f(1 + "foo") // arg would eval to error
    // error: expected 0 arguments, got 1
    //
    // above message kinda suggests the intepreter got 1 valid argument,
    // so 1) eval args, and only then 2) check arity; python does the same
    // rust shows both errors, but invalid args first, arity error second

    std::vector<std::shared_ptr<Object>> arg_vals; 
    for (auto& arg : m_args) {
        auto arg_val = arg->eval(interp);
        if (!arg_val)
            return {};
        arg_vals.push_back(arg_val);
    }

    if (callable.arity() != m_args.size()) {
        interp.error(std::format("expected {} arguments, got {}",
            callable.arity(), m_args.size()), m_text);
        return {};
    }
    return callable.__call__(arg_vals, interp);
}

std::shared_ptr<Object> FunctionExpr::eval(Interpreter& interp) const
{
    return std::make_shared<Function>(shared_from_this(), interp.scope_ptr(),
        interp.source());
}

bool ExpressionStmt::execute(Interpreter& interp) const
{
    if (auto val = m_expr->eval(interp)) {
        if (interp.is_repl_mode()) {
            auto str = val->__str__();
            if (val->is_string())
                str = escape(str);
            std::cout << str << '\n';
        }
        return true;
    }
    return false;
}

bool VarStmt::execute(Interpreter& interp) const
{
    std::shared_ptr<Object> val;
    if (m_init) {
        val = m_init->eval(interp);
        if (!val)
            return false;
    } else
        val = make_nil();
    assert(val);
    interp.scope().define_var(m_ident->name(), val);
    return true;
}

bool AssignStmt::execute(Interpreter& interp) const
{
    auto val = m_value->eval(interp);
    if (!val)
        return false;

    std::string_view name;
    if (m_place->is_identifier())
        name = std::static_pointer_cast<Identifier>(m_place)->name();
    assert(!name.empty());

    if (interp.scope().set_var(name, val))
        return true;

    if (m_place->is_identifier())
        interp.error(std::format("variable '{}' is not defined", name),
                     m_place->text());
    else
        assert(0);
    return false;
}

bool BlockStmt::execute(Interpreter& interp) const
{
    auto scope_change = interp.push_scope();
    auto res = execute_statements(m_stmts, interp);
    return res;
}

bool IfStmt::execute(Interpreter& interp) const
{
    auto val = m_test->eval(interp);
    if (!val)
        return false;
    if (!val->is_bool()) {
        interp.error(std::format("expected 'Bool', got '{}'", val->type_name()),
            m_test->text());
        return false;
    }

    if (val->get_bool())
        return m_then_block->execute(interp);
    if (m_else_block)
        return m_else_block->execute(interp);
    return true;
}

bool WhileStmt::execute(Interpreter& interp) const
{
    for (;;) {
        if (interp.check_interrupt())
            return false;

        auto val = m_test->eval(interp);
        if (!val)
            return false;
        if (!val->is_bool()) {
            interp.error(std::format("expected 'Bool', got '{}'",
                val->type_name()), m_test->text());
            return false;
        }
        if (!val->get_bool())
            break;

        assert(!interp.is_break());
        assert(!interp.is_continue());
        if (!m_block->execute(interp)) {
            if (interp.is_break()) {
                interp.set_break(false);
                break;
            }
            if (interp.is_continue()) {
                interp.set_continue(false);
                continue;
            }
            return false;
        }
    }
    return true;
}

bool ForStmt::execute(Interpreter& interp) const
{
    auto val = m_expr->eval(interp);
    if (!val)
        return false;

    if (!val->is_iterable()) {
        interp.error(std::format("'{}' is not iterable", val->type_name()),
                     m_expr->text());
        return false;
    }

    auto iter = val->__iter__();
    assert(iter);

    while (!iter->done()) {
        auto next = iter->next();
        if (!next)
            return false;

        assert(!interp.is_break());
        assert(!interp.is_continue());

        auto scope_change = interp.push_scope();
        interp.scope().define_var(m_ident->name(), next);
        auto res = execute_statements(m_block->statements(), interp);

        if (!res) {
            if (interp.is_break()) {
                interp.set_break(false);
                break;
            }
            if (interp.is_continue()) {
                interp.set_continue(false);
                continue;
            }
            return false;
        }
    }
    return true;
}

bool BreakStmt::execute(Interpreter& interp) const
{
    interp.set_break(true);
    return false; // "unwind" the stack until for/while loop code catches 'break'
}

bool ContinueStmt::execute(Interpreter& interp) const
{
    interp.set_continue(true);
    return false; // "unwind" the stack until for/while loop code catches 'continue'
}

bool FunctionDeclaration::execute(Interpreter& interp) const
{
    auto func = m_func->eval(interp);
    if (!func)
        return false;
    interp.scope().define_var(m_name->name(), func);
    return true;
}

bool ReturnStmt::execute(Interpreter& interp) const
{
    std::shared_ptr<Object> val;
    if (m_expr) {
        val = m_expr->eval(interp);
        if (!val)
            return false;
    } else
        val = make_nil();
    assert(val);

    interp.set_return_value(val);
    return false; // "unwind" the stack until function call code catches 'return'
}

bool Program::execute(Interpreter& interp) const
{
    for (auto& stmt : m_stmts) {
        if (interp.check_interrupt())
            return false;
        if (!stmt->execute(interp))
            return false;
    }
    return true;
}

void Scope::define_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(!name.empty());
    assert(value);
    m_vars[name] = value;
}

std::shared_ptr<Object> Scope::get_var(std::string_view name) const
{
    assert(!name.empty());
    for (auto scope = this; scope != nullptr; scope = scope->m_parent.get()) {
        if (auto pair = scope->m_vars.find(name); pair != scope->m_vars.end())
            return pair->second;
    }
    return {};
}

bool Scope::set_var(std::string_view name, std::shared_ptr<Object> value)
{
    assert(!name.empty());
    assert(value);
    for (auto scope = this; scope != nullptr; scope = scope->m_parent.get()) {
        if (auto pair = scope->m_vars.find(name); pair != scope->m_vars.end()) {
            pair->second = value;
            return true;
        }
    }
    return false;
}

void Interpreter::error(std::string msg, std::string_view span)
{
    m_errors.push_back({ std::move(msg), m_source, span });
}

void Interpreter::interpret(std::shared_ptr<Program> program)
{
    assert(program);

    m_errors.clear();
    m_source = program->text();
    assert(m_scope->is_top());
    program->execute(*this);
    assert(m_scope->is_top());
}

volatile std::sig_atomic_t g_interrupt;

bool Interpreter::check_interrupt()
{
    if (g_interrupt) {
        g_interrupt = 0;
        std::cerr << "interrupt\n";
        // when infinite loop does output to tty std::cout and SIGINT is
        // issued to stop that, failbit and badbit get set; clear those
        std::cout.clear();
        return true;
    }
    return false;
}

}
