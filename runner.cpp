#include "src/rpnlib.h"

#include <iostream>

template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T>
auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T>
reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

void dump_value(rpn_value& val) {
    switch (val.type) {
        case rpn_value::s32:
            std::cout << "s32 -> " << val.as_s32 << std::endl;
            break;
        case rpn_value::u32:
            std::cout << "u32 -> " << val.as_u32 << std::endl;
            break;
        case rpn_value::f64:
            std::cout << "f64 -> " << val.as_f64 << std::endl;
            break;
        case rpn_value::charptr:
            std::cout << "charptr -> \"" << val.as_charptr << "\"" << std::endl;
            break;
        case rpn_value::null:
            std::cout << "null" << std::endl;
            break;
    }
}

template<typename T>
void dump_variable(const T var) {
    std::cout << var->name << " = ";
    dump_value(*var->value.get());
}

void dump_variables(rpn_context & ctxt) {
    std::cout << "variables: " << ctxt.variables.size() << std::endl;
    for (auto variable : ctxt.variables) {
        dump_variable(variable);
    }
}

void dump_stack(rpn_context & ctxt) {
    std::cout << "stack: " << ctxt.stack.size() << std::endl;
    auto index = ctxt.stack.size();
    for (auto stack_val : ctxt.stack) {
        std::cout << --index << ": ";
        dump_value(*(stack_val.value.get()));
    }
}

void test_string() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    rpn_context ctxt;
    rpn_init(ctxt);
    rpn_debug([](rpn_context&, const char* message) {
        std::cout << "dbg: " << message << std::endl;
    });
    rpn_process(ctxt, "\"hello world\"");
    dump_stack(ctxt);
    rpn_clear(ctxt);
}

void test_assign() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    rpn_context ctxt;
    rpn_init(ctxt);
    rpn_operators_init(ctxt);
    rpn_debug([](rpn_context&, const char* message) {
        std::cout << "dbg: " << message << std::endl;
    });
    rpn_process(ctxt, "\"hello world\" $var =");
    dump_stack(ctxt);
    dump_variables(ctxt);
    rpn_clear(ctxt);
}

int main(int argc, char** argv) {
    test_string();
    test_assign();
}
