// check if arduino type works with ours

#include <rpnlib.h>
#include <rpnlib_util.h>

#include <ctime>
#include <cctype>
#include <string>
#include <iostream>
#include <iomanip>

const char* get_value_type(const rpn_stack_value& val) {
    switch (val.type) {
        case rpn_stack_value::Type::Value:
            return "VALUE";
        case rpn_stack_value::Type::Variable:
            return "VARIABLE";
        default:
            return "UNKNOWN";
    }
}

void dump_value(const rpn_value& val) {
    switch (val.type) {
        case rpn_value::Type::Boolean:
            std::cout << (val.toBoolean() ? "true" : "false") << " (Boolean) ";
            break;
        case rpn_value::Type::Integer:
            std::cout << val.toInt() << " (Integer) ";
            break;
        case rpn_value::Type::Unsigned:
            std::cout << val.toUint() << " (Unsigned) ";
            break;
        case rpn_value::Type::Float:
            std::cout << val.toFloat() << " (Float) ";
            break;
        case rpn_value::Type::String:
            std::cout << "\"" << val.toString().c_str() << "\" ";
            break;
        case rpn_value::Type::Error:
            std::cout << "error ";
            break;
        case rpn_value::Type::Null:
            std::cout << "null ";
            break;
    }
}

rpn_error dump_variables(rpn_context & ctxt) {
    std::cout << "variables: " << ctxt.variables.size() << std::endl;
    for (auto variable : ctxt.variables) {
        std::cout << "$" << variable.name.c_str() << " is ";
        if (!variable.value) {
            std::cout << "unset (error?)" << std::endl;
            continue;
        }
        dump_value(*variable.value.get());
    }
    return 0;
}

rpn_error dump_stack(rpn_context & ctxt) {
    size_t index = rpn_stack_size(ctxt);
    rpn_stack_foreach(ctxt, [&index](rpn_stack_value::Type, const rpn_value& value) {
        std::cout << std::setfill('0') << std::setw(3) << --index << ": ";
        dump_value(value);
        std::cout << " (" << get_value_type(value) << ")" << std::endl;
    });
    return 0;
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
    rpn_context ctxt;
    rpn_init(ctxt);
    rpn_operator_set(ctxt, "time", 0, [](rpn_context& c) -> rpn_error {
        rpn_value ts { rpn_int(time(nullptr)) };
        rpn_stack_push(c, ts);
        return 0;
    });
    rpn_operator_set(ctxt, "dump", 0, dump_stack);
    rpn_operator_set(ctxt, "vars", 0, dump_variables);
    rpn_operator_set(ctxt, "clear", 0, [](rpn_context& c) -> rpn_error {
        return rpn_stack_clear(c)
            ? rpn_operator_error::Ok
            : rpn_operator_error::CannotContinue;
    });
    rpn_operator_set(ctxt, "cast", 1, [](rpn_context& c) -> rpn_error {
        auto val = c.stack.back();
        c.stack.pop_back();
        rpn_value upd { (*val.value.get()).toFloat() };
        c.stack.push_back(upd);
        return 0;
    });
    rpn_operator_set(ctxt, "test", 0, [](rpn_context& c) -> rpn_error {
        test_concat(c);
        test_and(c);
        test_or(c);
        test_sum(c);
        return 0;
    });

    rpn_debug(ctxt, [](rpn_context&, const char* message) {
        std::cout << "DEBUG: " << message << std::endl;
    });

    while (true) {
        std::cout << ">>> ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << std::endl;
            break;
        }
        if (!rpn_process(ctxt, input.c_str())) {
            rpn_handle_error(ctxt.error, rpn_decode_errors([](const String& decoded) {
                std::cout << "ERROR! " << decoded.c_str() << std::endl;
            }));
        }
        dump_stack(ctxt);
        std::cout << std::endl;
    }
}
