/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

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
    RPN_TOKEN_NUMBER,
    RPN_TOKEN_STRING,
    RPN_TOKEN_VARIABLE,
};

using rpn_tokenizer_callback = std::function<bool(rpn_token_t, const char* ptr)>;

// something more useful than strtok
// based on https://stackoverflow.com/questions/9659697/parse-string-into-array-based-on-spaces-or-double-quotes-strings
// changes from the answer:
// - rework inner loop to call external function with token arg
// - keep quotes when parsing strings
// same as strtok, this still needs modifiable string. perhaps there is a way to not do that and pass some string-like struct with length from start_of_word to nullptr

void _rpn_tokenize(char* buffer, rpn_tokenizer_callback callback) {
    char *p = buffer;
    char *start_of_word = nullptr;

	char c = '\0';
	char tmp = '\0';

    enum states_t {
        UNKNOWN,
        IN_WORD,
        IN_NUMBER,
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
            if (!isdigit(c) && (c != '.')) {
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

// ----------------------------------------------------------------------------
// Functions methods
// ----------------------------------------------------------------------------

bool rpn_operator_set(rpn_context & ctxt, const char * name, unsigned char argc, bool (*callback)(rpn_context &)) {
    ctxt.operators.emplace_back(rpn_operator{name, argc, callback});
    return true;
}

bool rpn_operators_clear(rpn_context & ctxt) {
    ctxt.operators.clear();
    return true;
}

bool rpn_operators_init(rpn_context & ctxt) {

    rpn_operator_set(ctxt, "pi", 0, _rpn_pi);
    rpn_operator_set(ctxt, "e", 0, _rpn_e);

    rpn_operator_set(ctxt, "+", 2, _rpn_sum);
    rpn_operator_set(ctxt, "-", 2, _rpn_substract);
    rpn_operator_set(ctxt, "*", 2, _rpn_times);
    rpn_operator_set(ctxt, "/", 2, _rpn_divide);
    rpn_operator_set(ctxt, "mod", 2, _rpn_mod);
    rpn_operator_set(ctxt, "abs", 1, _rpn_abs);

    rpn_operator_set(ctxt, "round", 2, _rpn_round);
    rpn_operator_set(ctxt, "ceil", 1, _rpn_ceil);
    rpn_operator_set(ctxt, "floor", 1, _rpn_floor);
    rpn_operator_set(ctxt, "int", 1, _rpn_floor);

    #ifdef RPNLIB_ADVANCED_MATH
    rpn_operator_set(ctxt, "sqrt", 1, _rpn_sqrt);
    rpn_operator_set(ctxt, "log", 1, _rpn_log);
    rpn_operator_set(ctxt, "log10", 1, _rpn_log10);
    rpn_operator_set(ctxt, "exp", 1, _rpn_exp);
    rpn_operator_set(ctxt, "fmod", 2, _rpn_fmod);
    rpn_operator_set(ctxt, "pow", 2, _rpn_pow);
    rpn_operator_set(ctxt, "cos", 1, _rpn_cos);
    rpn_operator_set(ctxt, "sin", 1, _rpn_sin);
    rpn_operator_set(ctxt, "tan", 1, _rpn_tan);
    #endif

    rpn_operator_set(ctxt, "eq", 2, _rpn_eq);
    rpn_operator_set(ctxt, "ne", 2, _rpn_ne);
    rpn_operator_set(ctxt, "gt", 2, _rpn_gt);
    rpn_operator_set(ctxt, "ge", 2, _rpn_ge);
    rpn_operator_set(ctxt, "lt", 2, _rpn_lt);
    rpn_operator_set(ctxt, "le", 2, _rpn_le);

    rpn_operator_set(ctxt, "cmp", 2, _rpn_cmp);
    rpn_operator_set(ctxt, "cmp3", 3, _rpn_cmp3);
    rpn_operator_set(ctxt, "index", 1, _rpn_index);
    rpn_operator_set(ctxt, "map", 5, _rpn_map);
    rpn_operator_set(ctxt, "constrain", 3, _rpn_constrain);

    rpn_operator_set(ctxt, "and", 2, _rpn_and);
    rpn_operator_set(ctxt, "or", 2, _rpn_or);
    rpn_operator_set(ctxt, "xor", 2, _rpn_xor);
    rpn_operator_set(ctxt, "not", 1, _rpn_not);

    rpn_operator_set(ctxt, "dup", 1, _rpn_dup);
    rpn_operator_set(ctxt, "dup2", 2, _rpn_dup2);
    rpn_operator_set(ctxt, "swap", 2, _rpn_swap);
    rpn_operator_set(ctxt, "rot", 3, _rpn_rot);
    rpn_operator_set(ctxt, "unrot", 3, _rpn_unrot);
    rpn_operator_set(ctxt, "drop", 1, _rpn_drop);
    rpn_operator_set(ctxt, "over", 2, _rpn_over);
    rpn_operator_set(ctxt, "depth", 0, _rpn_depth);

    rpn_operator_set(ctxt, "exists", 1, _rpn_exists);
    rpn_operator_set(ctxt, "=", 2, _rpn_assign);
    rpn_operator_set(ctxt, "p", 1, _rpn_print);

    rpn_operator_set(ctxt, "ifn", 3, _rpn_ifn);
    rpn_operator_set(ctxt, "end", 1, _rpn_end);

    return true;
}

// ----------------------------------------------------------------------------
// Variables methods
// ----------------------------------------------------------------------------

// TODO: handle assignment in rpn_value class method
// TODO: avoid exposing rpn_value::as_... members
bool rpn_variable_set(rpn_context & ctxt, const char * name, double value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        if (v.value->type != rpn_value::f64) break;
        v.value->as_f64 = value;
        return true;
    }

    ctxt.variables.emplace_back(name, std::make_shared<rpn_value>(value));
    return true;
}

bool rpn_variable_get(rpn_context & ctxt, const char * name, float & value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        if (v.value->type != rpn_value::f64) break;
        value = v.value->as_f64;
        return true;
    }
    return false;
}

bool rpn_variable_del(rpn_context & ctxt, const char * name) {
    for (auto v = ctxt.variables.begin(); v != ctxt.variables.end(); v++) {
        if ((*v).name == name) {
            ctxt.variables.erase(v);
            return true;
        }
    }
    return false;
}

size_t rpn_variables_size(rpn_context & ctxt) {
    return ctxt.variables.size();
}

const char * rpn_variable_name(rpn_context & ctxt, unsigned char i) {
    if (i < ctxt.variables.size()) {
        return ctxt.variables[i].name.c_str();
    }
    return NULL;
}

bool rpn_variables_clear(rpn_context & ctxt) {
    ctxt.variables.clear();
    return true;
}

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
                _rpn_debug_callback(ctxt, "is string");
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(token));
                return true;
            case RPN_TOKEN_NUMBER:
                _rpn_debug_callback(ctxt, "is number");
                ctxt.stack.emplace_back(std::make_shared<rpn_value>(atof(token)));
                return true;
            case RPN_TOKEN_VARIABLE: {
                _rpn_debug_callback(ctxt, "is variable");
                auto var = std::find_if(ctxt.variables.begin(), ctxt.variables.end(), [token](const rpn_variable& v) {
                    return (v.name == token);
                });
                const bool found = (var != ctxt.variables.end());

                if (found) {
                    _rpn_debug_callback(ctxt, "existing variable");
                    ctxt.stack.emplace_back(RPN_STACK_TYPE_VARIABLE, (*var).value);
                    return true;
                }

                // no reason to continue
                if (!found && variable_must_exist) {
                    _rpn_debug_callback(ctxt, "variable does not exist");
                    rpn_error = RPN_ERROR_VARIABLE_DOES_NOT_EXIST;
                    return false;
                }

                // since we don't have the variable yet, push uninitialized one
                if (!found) {
                    _rpn_debug_callback(ctxt, "undefined variable");
                    auto null = std::make_shared<rpn_value>();
                    ctxt.variables.emplace_back(token, null);
                    ctxt.stack.emplace_back(RPN_STACK_TYPE_VARIABLE, null);
                    return true;
                }
            }

            case RPN_TOKEN_WORD:
            default:
                 _rpn_debug_callback(ctxt, "is word");
                 break;
        }

        // Is token an operator?
        {
            bool found = false;
            for (auto & f : ctxt.operators) {
                if (f.name == token) {
                    _rpn_debug_callback(ctxt, "is operator");
                    if (rpn_stack_size(ctxt) < f.argc) {
                        char buffer[64];
                        sprintf(buffer, "%s: func %u vs stack %u argc mismatch", f.name, rpn_stack_size(ctxt), f.argc);
                        _rpn_debug_callback(ctxt, buffer);
                        rpn_error = RPN_ERROR_ARGUMENT_COUNT_MISMATCH;
                        break;
                    }
                    if (!(f.callback)(ctxt)) {
                        _rpn_debug_callback(ctxt, "callback error");
                        // Method should set rpn_error
                        break;
                    }
                    _rpn_debug_callback(ctxt, "callback ok");
                    found = true;
                    break;
                }
            }
            if (RPN_ERROR_OK != rpn_error) return false;
            if (found) return true;
        }

        // Don't know the token
        _rpn_debug_callback(ctxt, "idk?");
        rpn_error = RPN_ERROR_UNKNOWN_TOKEN;
        return false;

    });

    // clean-up temporaries
    std::remove_if(ctxt.variables.begin(), ctxt.variables.end(), [](const rpn_variable& var) {
        return (var.value->type == rpn_value::null);
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
