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
    rpn_variables_foreach(ctxt, [](const String& name, rpn_value& value) {
        std::cout << "$" << name.c_str() << " is ";
        if (!value) {
            std::cout << "unset (error?)" << std::endl;
            return;
        }
        dump_value(value);
        std::cout << std::endl;
    });
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

int main(int argc, char** argv) {
    rpn_context ctxt;
    rpn_init(ctxt);

    rpn_operator_set(ctxt, "dump", 0, dump_stack);
    rpn_operator_set(ctxt, "vars", 0, dump_variables);
    rpn_operator_set(ctxt, "clear", 0, [](rpn_context& c) -> rpn_error {
        return rpn_stack_clear(c)
            ? rpn_operator_error::Ok
            : rpn_operator_error::CannotContinue;
    });

    rpn_operator_set(ctxt, "time", 0, [](rpn_context& c) -> rpn_error {
        rpn_value ts { rpn_int(time(nullptr)) };
        rpn_stack_push(c, ts);
        return 0;
    });

    rpn_operator_set(ctxt, "to_string", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toString() };
        rpn_stack_push(c, value);
        return value.toError();
    });
    rpn_operator_set(ctxt, "to_int", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toInt() };
        rpn_stack_push(c, value);
        return value.toError();
    });
    rpn_operator_set(ctxt, "to_uint", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toUint() };
        rpn_stack_push(c, value);
        return value.toError();
    });
    rpn_operator_set(ctxt, "to_boolean", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toBoolean() };
        rpn_stack_push(c, value);
        return value.toError();
    });
    rpn_operator_set(ctxt, "to_float", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toFloat() };
        rpn_stack_push(c, value);
        return value.toError();
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
