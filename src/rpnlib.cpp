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

rpn_error::rpn_error(int code) :
    category(rpn_error_category::Unknown),
    code(code)
{}

rpn_error::rpn_error(rpn_processing_error error) :
    category(rpn_error_category::Processing),
    code(static_cast<int>(error))
{}

rpn_error::rpn_error(rpn_operator_error error) :
    category(rpn_error_category::Operator),
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

rpn_error& rpn_error::operator =(rpn_operator_error error) {
    category = rpn_error_category::Operator;
    code = static_cast<int>(error);
    return *this;
}

rpn_error& rpn_error::operator =(rpn_value_error error) {
    category = rpn_error_category::Value;
    code = static_cast<int>(error);
    return *this;
}

bool rpn_error::operator ==(const rpn_error& other) {
    return (category == other.category) && (code == other.code);
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
    RPN_TOKEN_VARIABLE_REFERENCE,
    RPN_TOKEN_VARIABLE_VALUE
};

// Tokenizer based on https://stackoverflow.com/questions/9659697/parse-string-into-array-based-on-spaces-or-double-quotes-strings
// Changes from the answer:
// - rework inner loop to call external function with token arg
// - rework interal types. add variables, booleans and numbers
// - special condition for null
// - special condition for bool (true or false)
// - special condition for scientific notation
// - don't copy the string to insert '\0', just copy chars into a String
//
// TODO: support '\'' and '"' at the same time, we must have different branches for each one. (likely to be solved with goto, also reducing nesting)
// TODO: re2c could generate more efficient code for types, we don't have to check 3 times for bool

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
        return isdigit(c) != 0;
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

bool _rpn_token_is_bool(const char* token) {
    return (strcmp(token, "true") == 0) || (strcmp(token, "false") == 0);
}

// XXX: using out param since we don't know the length beforehand, and out is re-used

void _rpn_token_copy(const char* start, const char* stop, String& out) {
    out = "";
    const char* ptr = start;
    while (ptr != stop) {
        out += *ptr;
        ++ptr;
    }
}

// TODO: check that we have this signature:
// bool(rpn_token_t, const char* ptr)
// One option is to use std::inplace_function / function_ref:
// https://github.com/TartanLlama/function_ref

template <typename CallbackType>
void _rpn_tokenize(const char* buffer, String& token, CallbackType callback) {
    const char *p = buffer;
    const char *start_of_word = nullptr;

    char c = '\0';

    enum states_t {
        UNKNOWN,
        IN_NULL,
        IN_WORD,
        IN_NUMBER,
        IN_BOOLEAN,
        IN_STRING,
        IN_VARIABLE_REFERENCE,
        IN_VARIABLE_VALUE
    };

    rpn_token_t type = RPN_TOKEN_UNKNOWN;
    states_t state = UNKNOWN;

    while (true) {
        if (*p == '\0') {
            switch (state) {
            // Silently drop the rest, we must consistent syntax
            case IN_STRING:
                state = UNKNOWN;
                type = RPN_TOKEN_UNKNOWN;
                if (!callback(type, start_of_word)) {
                    goto stop_parsing;
                }
                break;
            case IN_VARIABLE_REFERENCE:
            case IN_VARIABLE_VALUE: {
                if (1 == (p - start_of_word)) {
                    state = UNKNOWN;
                    type = RPN_TOKEN_UNKNOWN;
                    if (!callback(type, start_of_word)) {
                        goto stop_parsing;
                    }
                }
                ++start_of_word;
                goto stop_parsing;
            }
            case UNKNOWN:
            case IN_NULL:
            case IN_WORD:
            case IN_NUMBER:
            case IN_BOOLEAN:
                break;
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

            // note: both STRING and VARIABLE ignore the first char
            //       post-actions must expect this
            if (c == '&') {
                state = IN_VARIABLE_REFERENCE;
                type = RPN_TOKEN_VARIABLE_REFERENCE;
            } else if (c == '$') {
                state = IN_VARIABLE_VALUE;
                type = RPN_TOKEN_VARIABLE_VALUE;
            } else if (c == '"') {
                state = IN_STRING;
                type = RPN_TOKEN_STRING;
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
                auto len = p - start_of_word;
                if (len > 1) {
                    token.reserve(len);
                    _rpn_token_copy(start_of_word + 1, p, token);
                } else {
                    token = "";
                }
                state = UNKNOWN;
                if (!callback(type, token)) {
                    goto stop_parsing;
                }
            }
            break;

        case IN_NUMBER:
        case IN_BOOLEAN:
        case IN_NULL:
            if (!isspace(c) && (
                (state == IN_NUMBER) ? !_rpn_token_still_number(c) :
                (state == IN_BOOLEAN) ? !_rpn_token_still_bool(c) :
                (state == IN_NULL) ? !_rpn_token_still_null(c) : true))
            {
                state = IN_WORD;
                type = RPN_TOKEN_WORD;
            }
            // fallthrough to the word

        case IN_WORD:
            if (isspace(c)) {
                token.reserve(p - start_of_word);
                _rpn_token_copy(start_of_word, p, token);
                state = UNKNOWN;
                if (!callback(type, token)) {
                    goto stop_parsing;
                }
            }
            break;

        case IN_VARIABLE_REFERENCE:
        case IN_VARIABLE_VALUE:
            if (isspace(c)) {
                auto len = p - start_of_word;
                if (len > 1) {
                    token.reserve(len);
                    _rpn_token_copy(start_of_word + 1, p, token);
                } else {
                    token = "";
                }
                state = UNKNOWN;
                if (!callback(type, token)) {
                    goto stop_parsing;
                }
            }
            break;

        }

        ++p;
    }

stop_parsing:

    if ((state != UNKNOWN) && (p - start_of_word)) {
        token.reserve(p - start_of_word);
        _rpn_token_copy(start_of_word, p, token);
        callback(type, token);
    }

}

} // namespace anonymous

// ----------------------------------------------------------------------------
// Main methods
// ----------------------------------------------------------------------------

bool rpn_process(rpn_context & ctxt, const char * input, bool variable_must_exist) {

    ctxt.error.reset();
    ctxt.input_buffer = "";

    _rpn_tokenize(input, ctxt.input_buffer, [&](rpn_token_t type, const String& token) {

        // Is token a string, bool, number, variable or null?
        switch (type) {

        case RPN_TOKEN_BOOLEAN: {
            if (_rpn_token_is_bool(token.c_str())) {
                ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(
                    _rpn_token_as_bool(token.c_str())
                ));
                return true;
            }
            break;
        }

        case RPN_TOKEN_NUMBER: {
            char* endptr = nullptr;
            rpn_float value = strtod(token.c_str(), &endptr);
            if (endptr == token.c_str() || endptr[0] != '\0') {
                break;
            }
            ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(value));
            return true;
        }

        case RPN_TOKEN_STRING:
            ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(token));
            return true;

        case RPN_TOKEN_VARIABLE_VALUE:
        case RPN_TOKEN_VARIABLE_REFERENCE: {
            if (!token.length()) {
                ctxt.error = rpn_processing_error::UnknownToken;
                return false;
            }
            auto var = std::find_if(ctxt.variables.cbegin(), ctxt.variables.cend(), [&token](const rpn_variable& v) {
                return (v.name == token);
            });
            const bool found = (var != ctxt.variables.end());

            // either push the reference to the value or the value itself, depending on the variable token type
            if (found) {
                if (RPN_TOKEN_VARIABLE_REFERENCE == type) {
                    ctxt.stack.get().emplace_back(rpn_stack_value::Type::Variable, (*var).value);
                } else if (RPN_TOKEN_VARIABLE_VALUE == type) {
                    ctxt.stack.get().emplace_back(*((*var).value));
                }
                return true;
            // in case we want value / explicitly said to check for variable existence
            } else if ((type == RPN_TOKEN_VARIABLE_VALUE) || variable_must_exist) {
                ctxt.error = rpn_processing_error::VariableDoesNotExist;
                return false;
            // since we don't have the variable yet, push uninitialized one
            } else {
                auto null = std::make_shared<rpn_value>();
                ctxt.variables.emplace_back(token, null);
                ctxt.stack.get().emplace_back(
                    rpn_stack_value::Type::Variable, null
                );
                return true;
            }
        }

        case RPN_TOKEN_NULL:
            if (_rpn_token_is_null(token.c_str())) {
                ctxt.stack.get().emplace_back(std::make_shared<rpn_value>());
                return true;
            }
            break;

        case RPN_TOKEN_UNKNOWN:
            ctxt.error = rpn_processing_error::UnknownToken;
            break;

        case RPN_TOKEN_WORD: {
            if (1 == token.length()) {
                switch (token[0]) {
                case '[':
                    ctxt.stack.stacks_push();
                    return true;
                case ']':
                    if (ctxt.stack.stacks_size() > 1) {
                        ctxt.stack.stacks_merge();
                        return true;
                    }
                    ctxt.error = rpn_processing_error::NoMoreStacks;
                    return false;
                }
            }
        }

        default:
            break;

        }

        // Is token an operator?
        {
            bool found = false;
            for (auto & f : ctxt.operators) {
                if (f.name == token) {
                    if (rpn_stack_size(ctxt) < f.argc) {
                        ctxt.error = rpn_operator_error::ArgumentCountMismatch;
                        break;
                    }
                    ctxt.error = (f.callback)(ctxt);
                    found = true;
                    break;
                }
            }
            if (ctxt.error.code) {
                return false;
            }
            if (found) return true;
        }

        // Don't know the token
        ctxt.error = rpn_processing_error::UnknownToken;
        return false;

    });

    // clean-up temporaries when
    // - variable is only referenced from the ctxt.variables (since we enforce shared_ptr copy, avoiding weak_ptr usage)
    // - value contents is either null or an error
    rpn_variables_unref(ctxt);

    return (0 == ctxt.error.code);

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
