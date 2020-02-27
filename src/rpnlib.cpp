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

// XXX REMOVE XXX ME XXX
#include <cstdio>

// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------

rpn_errors rpn_error = RPN_ERROR_OK;
rpn_debug_callback_f _rpn_debug_callback = nullptr;

// ----------------------------------------------------------------------------
// Utils
// ----------------------------------------------------------------------------

enum rpn_token_t {
    RPN_TOKEN_UNKNOWN,
    RPN_TOKEN_WORD,
    RPN_TOKEN_BOOLEAN,
    RPN_TOKEN_NUMBER,
    RPN_TOKEN_STRING,
    RPN_TOKEN_VARIABLE,
};

using rpn_tokenizer_callback = std::function<bool(rpn_token_t, const char* ptr)>;

// something more useful than strtok
// based on https://stackoverflow.com/questions/9659697/parse-string-into-array-based-on-spaces-or-double-quotes-strings
// changes from the answer:
// - rework inner loop to call external function with token arg
// - rework interal types to known about numbers and variables
// same as strtok, this still needs modifiable string. perhaps there is a way to not do that and pass some string-like struct with length from start_of_word to nullptr

namespace {

// convert raw word to bool. we only need to match one of `true` or `false`, since we expect tokenizer to deduce the correct string

bool _rpn_token_as_bool(const char* token) {
    if (strcmp(token, "true") == 0) {
        return true;
    }
    return false;
}

// after initial match, check if token still matches the originally deduced type

bool _rpn_token_is_number(char c) {
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

bool _rpn_token_is_bool(char c) {
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
void _rpn_tokenize(char* buffer, rpn_tokenizer_callback callback) {
    char *p = buffer;
    char *start_of_word = nullptr;

	char c = '\0';
	char tmp = '\0';

    enum states_t {
        UNKNOWN,
        IN_WORD,
        IN_NUMBER,
        IN_BOOLEAN,
        IN_STRING,
        IN_VARIABLE
    };

    rpn_token_t type = RPN_TOKEN_UNKNOWN;
    states_t state = UNKNOWN;

	while (true) {
		if (*p == '\0') break;

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
            if ((state == IN_NUMBER) && !isspace(c) && !_rpn_token_is_number(c)) {
                state = IN_WORD;
                type = RPN_TOKEN_WORD;
            }

        case IN_BOOLEAN:
            if ((state == IN_BOOLEAN) && !_rpn_token_is_bool(c)) {
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

    if (strlen(start_of_word) && (state != UNKNOWN)) {
        callback(type, start_of_word);
    }

}

} // namespace anonymous

// ----------------------------------------------------------------------------
// Main methods
// ----------------------------------------------------------------------------

bool rpn_process(rpn_context & ctxt, const char * input, bool variable_must_exist) {

    rpn_error = RPN_ERROR_OK;

    std::unique_ptr<char[]> _input_copy(strdup(input));

    _rpn_tokenize(_input_copy.get(), [&ctxt, variable_must_exist](rpn_token_t type, const char* token) {

        // Debug callback is always called first
        // TODO: pretty pointless on live device, error messages below should be improved instead
        if (_rpn_debug_callback) {
            _rpn_debug_callback(ctxt, token);
        }

        // Is token a word, string or variable?
        switch (type) {
            case RPN_TOKEN_STRING:
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(token));
                return true;
            case RPN_TOKEN_BOOLEAN:
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(_rpn_token_as_bool(token)));
            case RPN_TOKEN_NUMBER: {
                char* endptr = nullptr;
                double value = strtod(token, &endptr);
                if (endptr == token || endptr[0] != '\0') { 
                    break;
                }
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(value));
                return true;
            }
            case RPN_TOKEN_VARIABLE: {
                auto var = std::find_if(ctxt.variables.cbegin(), ctxt.variables.cend(), [token](const rpn_variable& v) {
                    return (v.name == token);
                });
                const bool found = (var != ctxt.variables.end());

                if (found) {
                    ctxt.stack.emplace_back(RPN_STACK_TYPE_VARIABLE, (*var).value);
                    return true;
                }

                // no reason to continue
                if (!found && variable_must_exist) {
                    rpn_error = RPN_ERROR_VARIABLE_DOES_NOT_EXIST;
                    return false;
                }

                // since we don't have the variable yet, push uninitialized one
                if (!found) {
                    auto null = std::make_shared<rpn_value>();
                    ctxt.variables.emplace_back(token, null);
                    ctxt.stack.emplace_back(RPN_STACK_TYPE_VARIABLE, null);
                    return true;
                }
            }

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
                        if (_rpn_debug_callback) {
                            char buffer[64];
                            sprintf(buffer, "%s: func %u vs stack %u argc mismatch", f.name.c_str(), rpn_stack_size(ctxt), f.argc);
                            _rpn_debug_callback(ctxt, buffer);
                        }
                        rpn_error = RPN_ERROR_ARGUMENT_COUNT_MISMATCH;
                        break;
                    }
                    if (!(f.callback)(ctxt)) {
                        // Method should set rpn_error
                        break;
                    }
                    found = true;
                    break;
                }
            }
            if (RPN_ERROR_OK != rpn_error) return false;
            if (found) return true;
        }

        // Don't know the token
        rpn_error = RPN_ERROR_UNKNOWN_TOKEN;
        return false;

    });

    // clean-up temporaries
    std::remove_if(ctxt.variables.begin(), ctxt.variables.end(), [](const rpn_variable& var) {
        return var.value->isNull();
    });

    return (RPN_ERROR_OK == rpn_error);

}

bool rpn_debug(rpn_debug_callback_f callback) {
    _rpn_debug_callback = callback;
    return true;
}

bool rpn_init(rpn_context & ctxt) {
    return rpn_operators_init(ctxt);
}

bool rpn_clear(rpn_context & ctxt) {
    rpn_operators_clear(ctxt);
    rpn_variables_clear(ctxt);
    rpn_stack_clear(ctxt);
    return true;
}
