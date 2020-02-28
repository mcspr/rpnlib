// check if arduino type works with ours

#include "src/rpnlib.h"

#include <cctype>
#include <string>
#include <iostream>

void dump_value(const rpn_value& val) {
    switch (val.type) {
        case rpn_value::boolean:
            std::cout << "boolean -> " << val.as_boolean << std::endl;
            break;
        case rpn_value::f64:
            std::cout << "f64 -> " << val.as_f64 << std::endl;
            break;
        case rpn_value::string:
            std::cout << "string -> \"" << val.as_string.c_str() << "\"" << std::endl;
            break;
        case rpn_value::null:
            std::cout << "null" << std::endl;
            break;
    }
}

bool dump_variables(rpn_context & ctxt) {
    std::cout << "variables: " << ctxt.variables.size() << std::endl;
    for (auto variable : ctxt.variables) {
        std::cout << "$" << variable.name.c_str() << " is ";
        if (!variable.value) {
            std::cout << "unset (error?)" << std::endl;
            continue;
        }
        dump_value(*variable.value.get());
    }
    return true;
}

bool dump_stack(rpn_context & ctxt) {
    std::cout << "stack: " << ctxt.stack.size() << std::endl;
    auto index = ctxt.stack.size();
    for (auto stack_val : ctxt.stack) {
        std::cout << --index << ": ";
        dump_value(*(stack_val.value.get()));
    }
    return true;
}

void test_concat(rpn_context & ctxt) {
    rpn_process(ctxt, "\"12345\" \"67890\" + p");
    rpn_stack_clear(ctxt);
}

void test_and(rpn_context & ctxt) {
    rpn_process(ctxt, "0 1 and");
    dump_stack(ctxt);
    rpn_stack_clear(ctxt);
}

void test_or(rpn_context & ctxt) {
    rpn_process(ctxt, "0 1 or");
    dump_stack(ctxt);
    rpn_stack_clear(ctxt);
}

void test_sum(rpn_context & ctxt) {
    rpn_process(ctxt, "2 2 +");
    dump_stack(ctxt);
    rpn_stack_clear(ctxt);
    rpn_process(ctxt, "5 2 * 3 + 5 mod");
    dump_stack(ctxt);
    rpn_stack_clear(ctxt);
}

int main(int argc, char** argv) {

    std::cout << "rpn_value " << sizeof(rpn_value) << std::endl;
    std::cout << "rpn_stack_value " << sizeof(rpn_stack_value) << std::endl;
    std::cout << "rpn_variable " << sizeof(rpn_variable) << std::endl;

    rpn_context ctxt;
    rpn_init(ctxt);
    rpn_operator_set(ctxt, "dump", 0, dump_stack);
    rpn_operator_set(ctxt, "vars", 0, dump_variables);
    rpn_operator_set(ctxt, "clear", 0, rpn_stack_clear);
    rpn_operator_set(ctxt, "cast", 1, [](rpn_context& c) {
        auto val = c.stack.back();
        c.stack.pop_back();
        auto upd = rpn_value(double(*val.value.get()));
        c.stack.push_back(upd);
        return true;
    });
    rpn_debug([](rpn_context&, const char* message) {
        std::cout << message << std::endl;
    });

    test_concat(ctxt);
    test_and(ctxt);
    test_or(ctxt);
    test_sum(ctxt);

    while (true) {
        std::cout << "> ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << std::endl;
            break;
        }
        rpn_process(ctxt, input.c_str());
        std::cout << std::endl;
    }
}
