// check if arduino type works with ours

#include <rpnlib.h>

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
            std::cout << (bool(val) ? "true" : "false") << " (Boolean) ";
            break;
        case rpn_value::Type::Integer:
            std::cout << rpn_int_t(val) << " (Integer) ";
            break;
        case rpn_value::Type::Unsigned:
            std::cout << rpn_uint_t(val) << " (Unsigned) ";
            break;
        case rpn_value::Type::Float:
            std::cout << rpn_float_t(val) << " (Float) ";
            break;
        case rpn_value::Type::String:
            std::cout << "\"" << String(val).c_str() << "\" ";
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
    return RPN_ERROR_OK;
}

rpn_error dump_stack(rpn_context & ctxt) {
    //std::cout << "stack: " << ctxt.stack.size() << std::endl;
    auto index = ctxt.stack.size();
    for (auto it = ctxt.stack.rbegin() ; it != ctxt.stack.rend(); ++it) {
        std::cout << std::setfill('0') << std::setw(3) << --index << ": ";
        dump_value(*((*it).value.get()));
        std::cout << " (" << get_value_type(*it) << ")" << std::endl;
    }
    return RPN_ERROR_OK;
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

std::string _rpn_error_describe(rpn_error error) {
    std::string out;

    switch (error.category) {

    case rpn_error_category::Processing: {
       switch (static_cast<rpn_processing_error>(error.code)) {
       case RPN_ERROR_OK:
           out = "OK";
           break;
       case RPN_ERROR_UNKNOWN_TOKEN:
           out = "Unknown token";
           break;
       case RPN_ERROR_ARGUMENT_COUNT_MISMATCH:
           out = "Argument mismatch";
           break;
       case RPN_ERROR_INVALID_ARGUMENT:
           out = "Invalid argument";
           break;
       case RPN_ERROR_VARIABLE_DOES_NOT_EXIST:
           out = "Variable does not exist";
           break;
       }
       break;
    }

    case rpn_error_category::Value:
       break;

    case rpn_error_category::Unknown:
       break;

    }

    if (!out.length()) {
        out = "Category ";
        out += std::to_string(static_cast<int>(error.category));
        out += ", Code ";
        out += std::to_string(error.code);
    }

    return out;
}

int main(int argc, char** argv) {
    rpn_context ctxt;
    rpn_init(ctxt);
    rpn_operator_set(ctxt, "time", 0, [](rpn_context& c) -> rpn_error {
        rpn_value ts { rpn_int_t(time(nullptr)) };
        rpn_stack_push(c, ts);
        return RPN_ERROR_OK;
    });
    rpn_operator_set(ctxt, "dump", 0, dump_stack);
    rpn_operator_set(ctxt, "vars", 0, dump_variables);
    rpn_operator_set(ctxt, "clear", 0, [](rpn_context& c) -> rpn_error {
        return rpn_stack_clear(c) ? RPN_ERROR_OK : RPN_ERROR_VALUE;
    });
    rpn_operator_set(ctxt, "cast", 1, [](rpn_context& c) -> rpn_error {
        auto val = c.stack.back();
        c.stack.pop_back();
        rpn_value upd { rpn_float_t(*val.value.get()) };
        c.stack.push_back(upd);
        return RPN_ERROR_OK;
    });
    rpn_operator_set(ctxt, "test", 0, [](rpn_context& c) -> rpn_error {
        test_concat(c);
        test_and(c);
        test_or(c);
        test_sum(c);
        return RPN_ERROR_OK;
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
            std::cout << "ERROR! "  << _rpn_error_describe(ctxt.error) << std::endl;
        }
        dump_stack(ctxt);
        std::cout << std::endl;
    }
}
