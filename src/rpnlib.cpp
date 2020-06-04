/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

The rpnlib library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The rpnlib library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the rpnlib library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "rpnlib.h"
#include "rpnlib_stack.h"
#include "rpnlib_value.h"
#include "rpnlib_variable.h"
#include "rpnlib_operators.h"

#include <algorithm>
#include <functional>
#include <vector>

#include <cstring>
#include <cstdlib>
#include <cctype>

// ----------------------------------------------------------------------------
// Errors
// ----------------------------------------------------------------------------

rpn_error::rpn_error() :
    category(rpn_error_category::Unknown),
    code(0)
{}

rpn_error::rpn_error(rpn_processing_error error) :
    category(rpn_error_category::Processing),
    code(static_cast<int>(error))
{}

rpn_error::rpn_error(rpn_value_error error) :
    category(rpn_error_category::Value),
    code(static_cast<int>(error))
{}

rpn_error& rpn_error::operator =(rpn_processing_error error) {
    category = rpn_error_category::Processing;
    code = static_cast<int>(error);
    return *this;
}

rpn_error& rpn_error::operator =(rpn_value_error error) {
    category = rpn_error_category::Value;
    code = static_cast<int>(error);
    return *this;
}

void rpn_error::reset() {
    category = rpn_error_category::Unknown;
    code = 0;
}

// ----------------------------------------------------------------------------
// Utils
// ----------------------------------------------------------------------------

enum rpn_token_t {
    RPN_TOKEN_UNKNOWN,
    RPN_TOKEN_NULL,
    RPN_TOKEN_WORD,
    RPN_TOKEN_BOOLEAN,
    RPN_TOKEN_NUMBER,
    RPN_TOKEN_STRING,
    RPN_TOKEN_VARIABLE,
};

// Tokenizer based on https://stackoverflow.com/questions/9659697/parse-string-into-array-based-on-spaces-or-double-quotes-strings
// Changes from the answer:
// - rework inner loop to call external function with token arg
// - rework interal types. add variables, booleans and numbers
// - special condition for bool
// - special condition for scientific notation
// String is modified in-place by inserting '\0', allowing to use callback on the resulting pointer directly.
// Perhaps, there is a way to not do that and pass some string-like struct with length from start_of_word to nullptr.
//
// TODO: support '\'' and '"' at the same time, we must have different branches for each one. (likely to be solved with goto, also reducing nesting)
// TODO: re2c could generate more efficient code

namespace {

// convert raw word to bool. we only need to match one of `true` or `false`, since we expect tokenizer to deduce the correct string

bool _rpn_token_as_bool(const char* token) {
    return (strcmp(token, "true") == 0);
}

// after initial match, check if token still matches the originally deduced type
// no need for further checks, callback is expected to validate the token
// TODO: ...should it though?

bool _rpn_token_still_number(char c) {
    switch (c) {
    case '.':
    case 'e':
    case 'E':
    case '-':
    case '+':
        return true;
    default:
        return isdigit(c);
    }
}

bool _rpn_token_still_bool(char c) {
    switch (c) {
    case 'r':
    case 'u':
    case 'e':
    case 'a':
    case 'l':
    case 's':
        return true;
    default:
        return false;
    }
}

bool _rpn_token_still_null(char c) {
    switch (c) {
    case 'n':
    case 'u':
    case 'l':
        return true;
    default:
        return false;
    }
}

bool _rpn_token_is_null(const char* token) {
    return (strcmp(token, "null") == 0);
}

// TODO: check that we have this signature:
// bool(rpn_token_t, const char* ptr)
// One option is to use std::inplace_function / function_ref:
// https://github.com/TartanLlama/function_ref

template <typename CallbackType>
void _rpn_tokenize(char* buffer, CallbackType callback) {
    char *p = buffer;
    char *start_of_word = nullptr;

    char c = '\0';
    char tmp = '\0';

    enum states_t {
        UNKNOWN,
        IN_NULL,
        IN_WORD,
        IN_NUMBER,
        IN_BOOLEAN,
        IN_STRING,
        IN_VARIABLE
    };

    rpn_token_t type = RPN_TOKEN_UNKNOWN;
    states_t state = UNKNOWN;

    while (true) {
        if (*p == '\0') {
            // Silently drop, we must have closing quote
            if (state == IN_STRING) {
                state = UNKNOWN;
                type = RPN_TOKEN_UNKNOWN;
                callback(type, start_of_word);
            }
            break;
        }

        c = *p;
        switch (state) {
        case UNKNOWN:
            if (isspace(c)) {
                ++p;
                continue;
            }

            // both STRING and VARIABLE can ignore the first char
            if (c == '$') {
                state = IN_VARIABLE;
                type = RPN_TOKEN_VARIABLE;
                ++p;
            } else if (c == '"') {
                state = IN_STRING;
                type = RPN_TOKEN_STRING;
                ++p;
            } else if (isdigit(c) || (c == '-') || (c == '+')) {
                state = IN_NUMBER;
                type = RPN_TOKEN_NUMBER;
            } else if ((c == 't') || (c == 'f')) {
                state = IN_BOOLEAN;
                type = RPN_TOKEN_BOOLEAN;
            } else if (c == 'n') {
                state = IN_NULL;
                type = RPN_TOKEN_NULL;
            } else {
                state = IN_WORD;
                type = RPN_TOKEN_WORD;
            }
            start_of_word = p;
            break;

        case IN_STRING:
            if (c == '"') {
                tmp = *p;
                *p = '\0';
                if (!callback(type, start_of_word)) {
                    state = UNKNOWN;
                    break;
                }
                *p = tmp;
                state = UNKNOWN;
            }
            break;

        case IN_NUMBER:
        case IN_BOOLEAN:
        case IN_NULL:
            if (!isspace(c) && (
                (state == IN_NUMBER) ? !_rpn_token_still_number(c) :
                (state == IN_BOOLEAN) ? !_rpn_token_still_bool(c) :
                (state == IN_NULL) ? !_rpn_token_still_null(c) : true)) {
                state = IN_WORD;
                type = RPN_TOKEN_WORD;
            }

        case IN_VARIABLE:
        case IN_WORD:
            if (isspace(c)) {
                tmp = *p;
                *p = '\0';
                if (!callback(type, start_of_word)) {
                    state = UNKNOWN;
                    break;
                }
                *p = tmp;
                state = UNKNOWN;
            }
            break;
        }

        ++p;
    }

    if ((state != UNKNOWN) && strlen(start_of_word)) {
        callback(type, start_of_word);
    }

}

} // namespace anonymous

// ----------------------------------------------------------------------------
// Main methods
// ----------------------------------------------------------------------------

bool rpn_process(rpn_context & ctxt, const char * input, bool variable_must_exist) {

    ctxt.input_buffer = input;
    ctxt.error.reset();

    _rpn_tokenize(const_cast<char*>(ctxt.input_buffer.c_str()), [&ctxt, variable_must_exist](rpn_token_t type, const char* token) {

        // Is token a string, bool, number, variable or null?
        switch (type) {
            case RPN_TOKEN_STRING:
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(token));
                return true;

            case RPN_TOKEN_BOOLEAN:
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(_rpn_token_as_bool(token)));
                return true;

            case RPN_TOKEN_NUMBER: {
                char* endptr = nullptr;
                rpn_float_t value = strtod(token, &endptr);
                if (endptr == token || endptr[0] != '\0') { 
                    break;
                }
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(value));
                return true;
            }

            case RPN_TOKEN_VARIABLE: {
                if (!strlen(token)) {
                    break;
                }
                auto var = std::find_if(ctxt.variables.cbegin(), ctxt.variables.cend(), [token](const rpn_variable& v) {
                    return (v.name == token);
                });
                const bool found = (var != ctxt.variables.end());

                if (found) {
                    ctxt.stack.emplace_back(rpn_stack_value::Type::Variable, (*var).value);
                    return true;
                }

                // no reason to continue
                if (!found && variable_must_exist) {
                    ctxt.error = RPN_ERROR_VARIABLE_DOES_NOT_EXIST;
                    return false;
                }

                // since we don't have the variable yet, push uninitialized one
                if (!found) {
                    auto null = std::make_shared<rpn_value>();
                    ctxt.variables.emplace_back(
                        String(token), null
                    );
                    ctxt.stack.emplace_back(
                        rpn_stack_value::Type::Variable, null
                    );
                    return true;
                }
            }

            case RPN_TOKEN_NULL:
                if (_rpn_token_is_null(token)) {
                    ctxt.stack.emplace_back(std::make_shared<rpn_value>());
                    return true;
                }
                break;

            case RPN_TOKEN_UNKNOWN:
                ctxt.error = RPN_ERROR_UNKNOWN_TOKEN;
                break;

            case RPN_TOKEN_WORD:
            default:
                break;
        }

        // Is token an operator?
        {
            bool found = false;
            for (auto & f : ctxt.operators) {
                if (f.name == token) {
                    if (rpn_stack_size(ctxt) < f.argc) {
                        ctxt.error = RPN_ERROR_ARGUMENT_COUNT_MISMATCH;
                        break;
                    }
                    ctxt.error = (f.callback)(ctxt);
                    found = true;
                    break;
                }
            }
            if (ctxt.error.code) return false;
            if (found) return true;
        }

        // Don't know the token
        ctxt.error = RPN_ERROR_UNKNOWN_TOKEN;
        return false;

    });

    // clean-up temporaries
    ctxt.variables.erase(
        std::remove_if(ctxt.variables.begin(), ctxt.variables.end(), [](const rpn_variable& var) {
            return ((var.value.use_count() == 1) && var.value->isNull());
        }),
        ctxt.variables.end()
    );

    return (!ctxt.error.code);

}

bool rpn_debug(rpn_context & ctxt, rpn_debug_callback_f callback) {
    ctxt.debug_callback = callback;
    return true;
}

bool rpn_init(rpn_context & ctxt) {
    ctxt.input_buffer.reserve(RPNLIB_EXPRESSION_BUFFER_SIZE);
    return rpn_operators_init(ctxt);
}

bool rpn_clear(rpn_context & ctxt) {
    rpn_operators_clear(ctxt);
    rpn_variables_clear(ctxt);
    rpn_stack_clear(ctxt);
    return true;
}
