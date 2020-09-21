// check if arduino type works with ours

#include <rpnlib.h>

#include <ctime>
#include <cctype>
#include <string>
#include <iostream>
#include <iomanip>

const char* stack_type(rpn_stack_value::Type type) {
    switch (type) {
        case rpn_stack_value::Type::Value:
            return "VALUE";
        case rpn_stack_value::Type::Variable:
            return "VARIABLE";
        case rpn_stack_value::Type::VariableValueName:
            return "VARIABLE NAME";
        case rpn_stack_value::Type::VariableReferenceName:
            return "REFERENCE NAME";
        case rpn_stack_value::Type::OperatorName:
            return "OPERATOR NAME";
        case rpn_stack_value::Type::StackKeyword:
            return "STACK KEYWORD";
        case rpn_stack_value::Type::Array:
            return "ARRAY SIZE";
        case rpn_stack_value::Type::Block:
            return "BLOCK SIZE";
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
            std::cout << val.toString().c_str() << " (String) ";
            break;
        case rpn_value::Type::Error:
            std::cout << "error ";
            break;
        case rpn_value::Type::Null:
            std::cout << "null ";
            break;
    }
}

void dump_variables(rpn_context & ctxt) {
    rpn_variables_foreach(ctxt, [](const String& name, rpn_value& value) {
        std::cout << "$" << name.c_str() << " is ";
        dump_value(value);
        std::cout << std::endl;
    });
}

void dump_stack(rpn_context & ctxt) {
    std::cout << "REASON: " << (
        (ctxt.stack.stacks_reason() == rpn_nested_stack::Reason::Main) ? "Main" :
        (ctxt.stack.stacks_reason() == rpn_nested_stack::Reason::Array) ? "Array" :
        (ctxt.stack.stacks_reason() == rpn_nested_stack::Reason::Block) ? "Block" :
        "UNKNOWN!"
    ) << " DEPTH: " << ctxt.stack.stacks_size() << std::endl;
    size_t index = rpn_stack_size(ctxt);
    rpn_stack_foreach(ctxt, [&index](unsigned char block, rpn_stack_value::Type type, const rpn_value& value) {
        std::cout << std::setfill('0') << std::setw(3) << --index << ": ";
        std::cout << std::setfill('0') << std::setw(3) << static_cast<int>(block) << ": ";
        dump_value(value);
        std::cout << " (" << stack_type(type) << ")" << std::endl;
    });
    std::cout << std::endl;
}

void dump_operators(rpn_context & ctxt) {
    size_t index = 0;
    rpn_operators_foreach(ctxt, [&index](const String& name, size_t argc, rpn_operator::callback_type) {
        std::cout << std::setfill('0') << std::setw(3) << ++index << ": ";
        std::cout << name.c_str() << "(...), " << argc << std::endl;
    });
}

void dump_state(rpn_context & ctxt) {
    dump_stack(ctxt);
    dump_variables(ctxt);
}

int main(int argc, char** argv) {
    rpn_context ctxt;
    rpn_init(ctxt);

    rpn_operator_set(ctxt, "clear", 0, [](rpn_context& c) -> rpn_error {
        return rpn_stack_clear(c)
            ? rpn_operator_error::Ok
            : rpn_operator_error::CannotContinue;
    });

    rpn_operator_set(ctxt, "operators", 0, [](rpn_context& c) -> rpn_error {
        dump_operators(c);
        return 0;
    });

    rpn_operator_set(ctxt, "time", 0, [](rpn_context& c) -> rpn_error {
        rpn_value ts { rpn_int(time(nullptr)) };
        rpn_stack_push(c, ts);
        return 0;
    });

    rpn_operator_set(ctxt, "to_string", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toString() };
        rpn_stack_push(c, value);
        return 0;
    });
    rpn_operator_set(ctxt, "to_boolean", 1, [](rpn_context& c) -> rpn_error {
        rpn_value value { rpn_stack_pop(c).toBoolean() };
        rpn_stack_push(c, value);
        return 0;
    });

    rpn_operator_set(ctxt, "to_int", 1, [](rpn_context& c) -> rpn_error {
        auto value = rpn_stack_pop(c);
        auto conversion = value.checkedToInt();
        if (!conversion.ok()) {
            return conversion.error();
        }

        rpn_stack_push(c, rpn_value(conversion.value()));
        return 0;
    });
    rpn_operator_set(ctxt, "to_uint", 1, [](rpn_context& c) -> rpn_error {
        auto value = rpn_stack_pop(c);
        auto conversion = value.checkedToUint();
        if (!conversion.ok()) {
            return conversion.error();
        }

        rpn_stack_push(c, rpn_value(conversion.value()));
        return 0;
    });
    rpn_operator_set(ctxt, "to_float", 1, [](rpn_context& c) -> rpn_error {
        auto value = rpn_stack_pop(c);
        auto conversion = value.checkedToFloat();
        if (!conversion.ok()) {
            return conversion.error();
        }

        rpn_stack_push(c, rpn_value(conversion.value()));
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
            auto handler = [&ctxt](const String& decoded) {
                auto pos = ctxt.error.position;
                std::cout << "    ";
                if (pos > 0) {
                    while (--pos) {
                        std::cout << ' ';
                    }
                }
                std::cout << "^\n";
                std::cout << "ERR: " << decoded.c_str() << std::endl;
            };
            rpn_handle_error(ctxt.error, rpn_decode_errors(handler));
        }
        dump_state(ctxt);
        std::cout << std::endl;
    }
}
