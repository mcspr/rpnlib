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
#include <memory>
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

using rpn_tokenizer_callback = std::function<bool(const char* ptr)>;

bool _rpn_is_number(const char * s) {
    unsigned char len = strlen(s);
    if (0 == len) return false;
    bool decimal = false;
    bool digit = false;
    for (unsigned char i=0; i<len; i++) {
        if (('-' == s[i]) || ('+' == s[i])) {
            if (i>0) return false;
        } else if (s[i] == '.') {
            if (!digit) return false;
            if (decimal) return false;
            decimal = true;
        } else if (!isdigit(s[i])) {
            return false;
        } else {
            digit = true;
        }
    }
    return digit;
}

bool _rpn_is_string(const char * s) {
    const auto len = strlen(s);
    return (len && len > 2 && (s[0] == '"') && (s[len - 1] == '"'));
}

void _rpn_tokenize(char* buffer, rpn_tokenizer_callback callback) {
    char *p = buffer;
    char *start_of_word = nullptr;

	char c = '\0';
	char tmp = '\0';

    enum states { DULL, IN_WORD, IN_STRING } state = DULL;

	while (true) {
		if (*p == '\0') break;

        c = *p;
        switch (state) {
        case DULL:
            if (isspace(c)) {
                continue;
            }

            if (c == '"') {
                state = IN_STRING;
                start_of_word = p; 
                break;
            }
            state = IN_WORD;
            start_of_word = p;
            break;

        case IN_STRING:
            if (c == '"') {
                ++p;
                tmp = *p;
                *p = '\0';
				if (!callback(start_of_word)) break;
                *p = tmp;
                state = DULL;
            }
            break;

        case IN_WORD:
            if (isspace(c)) {
                tmp = *p;
                *p = '\0';
				if (!callback(start_of_word)) break;
                *p = tmp;
                state = DULL;
            }
            break;
        }

		++p;
    }

    if (strlen(start_of_word) && (state != DULL)) {
        callback(start_of_word);
    }

}

// ----------------------------------------------------------------------------
// Stack methods
// ----------------------------------------------------------------------------

rpn_variable::rpn_variable(const rpn_variable& other) :
    name(strdup(other.name))
{}

rpn_variable::rpn_variable(rpn_variable&& other) {
    name = other.name;
    other.name = nullptr;
}

rpn_variable::~rpn_variable() {
    free(name);
}

rpn_variable& rpn_variable::operator=(const rpn_variable& other) {
    if (name) free(name);
    name = strdup(other.name);
    return *this;
}

rpn_value::rpn_value() :
    type(rpn_value::unknown)
{}

rpn_value::rpn_value(const rpn_value& other) {
    if (type == rpn_value::charptr) {
        free(as_charptr);
    }
    if (other.type == rpn_value::charptr) {
        as_charptr = strdup(other.as_charptr);
    }
    type = other.type;
}

rpn_value::rpn_value(rpn_value&& other) {
    if (type == rpn_value::charptr) {
        free(as_charptr);
    }
    if (other.type == rpn_value::charptr) {
        as_charptr = other.as_charptr;
        other.as_charptr = nullptr;
    }
    type = other.type;
    other.type = rpn_value::unknown;
}

rpn_value::rpn_value(int32_t value) :
    as_s32(value),
    type(rpn_value::s32)
{}

rpn_value::rpn_value(uint32_t value) :
    as_u32(value),
    type(rpn_value::u32)
{}

rpn_value::rpn_value(double value) :
    as_f64(value),
    type(rpn_value::f64)
{}

rpn_value& rpn_value::operator=(const rpn_value& other) {
    type = other.type;

    switch (other.type) {
        case rpn_value::s32:
            as_s32 = other.as_s32;
            break;
        case rpn_value::u32:
            as_u32 = other.as_u32;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::charptr:
            as_charptr = strdup(other.as_charptr);
            break;
    }
            
    return *this;
}

rpn_value::rpn_value(char* value) :
    as_charptr(strdup(value)),
    type(rpn_value::charptr)
{}

rpn_value::~rpn_value() {
    if (type == rpn_value::charptr) {
        free(as_charptr);
    }
}

rpn_value::operator int32_t() const {
    if (type == rpn_value::s32) {
        return as_s32;
    }
    return 0;
}

rpn_value::operator uint32_t() const {
    if (type == rpn_value::u32) {
        return as_u32;
    }
    return 0;
}

rpn_value::operator double() const {
    if (type == rpn_value::f64) {
        return as_f64;
    }
    return 0.0L;
}

rpn_value::operator char*() const {
    if (type == rpn_value::charptr) {
        return as_charptr;
    }
    return nullptr;
}

rpn_stack_value::rpn_stack_value(const rpn_value& value, rpn_variable* variable) :
    variable(variable),
    value(value)
{}

rpn_stack_value::rpn_stack_value(const rpn_value& value) :
    rpn_stack_value(value, nullptr)
{}

rpn_stack_value::rpn_stack_value(rpn_variable* variable) :
    variable(variable),
    value(variable->value)
{}

unsigned char rpn_stack_size(rpn_context & ctxt) {
    return ctxt.stack.size();
}

bool rpn_stack_clear(rpn_context & ctxt) {
    ctxt.stack.clear();
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, float value) {
    ctxt.stack.emplace_back(rpn_value{double(value)});
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, double value) {
    ctxt.stack.emplace_back(rpn_value{value});
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, int32_t value) {
    ctxt.stack.emplace_back(rpn_value{value});
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, uint32_t value) {
    ctxt.stack.emplace_back(rpn_value{value});
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, char* value) {
    ctxt.stack.emplace_back(rpn_value{value});
    return true;
}

bool rpn_stack_pop(rpn_context & ctxt, float & value) {
    if (0 == ctxt.stack.size()) return false;
    auto& ref = ctxt.stack.back();
    if (ref.value.type == rpn_value::f64) {
        value = ref.value.as_f64;
    } else {
        value = 0;
    }
    ctxt.stack.pop_back();
    return true;
}

bool rpn_stack_pop(rpn_context & ctxt, char** value) {
    if (0 == ctxt.stack.size()) return false;
    *value = ctxt.stack.back().value;
    ctxt.stack.pop_back();
    return true;
}

bool rpn_stack_get(rpn_context & ctxt, unsigned char index, float & value) {
    unsigned char size = ctxt.stack.size();
    if (index >= size) return false;
    auto& ref = ctxt.stack.at(size-index-1);
    if (ref.value.type == rpn_value::f64) {
        value = ref.value.as_f64;
    } else {
        value = 0.0;
    }
    return true;
}

// ----------------------------------------------------------------------------
// Functions methods
// ----------------------------------------------------------------------------

bool rpn_operator_set(rpn_context & ctxt, const char * name, unsigned char argc, bool (*callback)(rpn_context &)) {
    ctxt.operators.emplace_back(rpn_operator{strdup(name), argc, callback});
    return true;
}

bool rpn_operators_clear(rpn_context & ctxt) {
    for (auto & v : ctxt.operators) {
        free(v.name);
    }
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

    rpn_operator_set(ctxt, "ifn", 3, _rpn_ifn);
    rpn_operator_set(ctxt, "end", 1, _rpn_end);

    return true;
}

// ----------------------------------------------------------------------------
// Variables methods
// ----------------------------------------------------------------------------

// TODO: handle charptr lifetime in rpn_value class
bool rpn_variable_set(rpn_context & ctxt, const char * name, float value) {
    for (auto & v : ctxt.variables) {
        if (strcmp(v.name, name) != 0) continue;
        if (v.value.type != rpn_value::f64) break;
        v.value.as_f64 = value;
        return true;
    }
    rpn_variable variable;
    variable.name = strdup(name);
    variable.value = rpn_value(double(value));
    ctxt.variables.emplace_back(std::move(variable));
    return true;
}

bool rpn_variable_get(rpn_context & ctxt, const char * name, float & value) {
    for (auto & v : ctxt.variables) {
        if (strcmp(v.name, name) != 0) continue;
        if (v.value.type != rpn_value::f64) break;
        value = v.value.as_f64;
        return true;
    }
    return false;
}

bool rpn_variable_del(rpn_context & ctxt, const char * name) {
    for (auto v = ctxt.variables.begin(); v != ctxt.variables.end(); v++) {
        if (strcmp((*v).name, name) == 0) {
            ctxt.variables.erase(v);
            return true;
        }
    }
    return false;
}

unsigned char rpn_variables_size(rpn_context & ctxt) {
    return ctxt.variables.size();
}

char * rpn_variable_name(rpn_context & ctxt, unsigned char i) {
    if (i < ctxt.variables.size()) {
        return ctxt.variables[i].name;
    }
    return NULL;
}

bool rpn_variables_clear(rpn_context & ctxt) {
    for (auto & v : ctxt.variables) {
        free(v.name);
    }
    ctxt.variables.clear();
    return true;
}

// ----------------------------------------------------------------------------
// Main methods
// ----------------------------------------------------------------------------

bool rpn_process(rpn_context & ctxt, const char * input, bool variable_must_exist) {

    rpn_error = RPN_ERROR_OK;

    std::unique_ptr<char[]> _input_copy(strdup(input));

    _rpn_tokenize(_input_copy.get(), [&ctxt, variable_must_exist](const char* token) {
        
        // Debug callback
        if (_rpn_debug_callback) {
            _rpn_debug_callback(ctxt, token);
        }

        // Multiple spaces
        if (0 == strlen(token)) {
            _rpn_debug_callback(ctxt, "is space");
            return true;
        }

        // Is token a (quoted) string?
        if (_rpn_is_string(token)) {
            _rpn_debug_callback(ctxt, "is string");
            // TODO: implement generic copy func
            rpn_value val;
            val.type = rpn_value::charptr;
            val.as_charptr = (char*)malloc(strlen(token) - 2 + 1);
            memcpy(val.as_charptr, token + 1, strlen(token) - 2);
            val.as_charptr[strlen(token) - 2] = '\0';
            _rpn_debug_callback(ctxt, "string without quotes:");
            _rpn_debug_callback(ctxt, val.as_charptr);
            ctxt.stack.emplace_back(val);
            char buffer[64];
            sprintf(buffer, "stack has %u values\n", ctxt.stack.size());
            _rpn_debug_callback(ctxt, buffer);
            return true;
        }

        // Is token a number?
        if (_rpn_is_number(token)) {
            _rpn_debug_callback(ctxt, "is number");
            ctxt.stack.emplace_back(rpn_value{atof(token)});
            return true;
        }

        // Is token a variable?
        if (token[0] == '$') {
            _rpn_debug_callback(ctxt, "is variable");
            const char* name = token + 1;
            auto var = std::find_if(ctxt.variables.begin(), ctxt.variables.end(), [name](const rpn_variable& var) {
                return (strcmp(var.name, name) == 0);
            });

            const bool found = (var != ctxt.variables.end());

            if (found) {
                _rpn_debug_callback(ctxt, "existing variable");
                ctxt.stack.emplace_back(&(*var));
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
                rpn_variable variable;
                variable.name = strdup(name);
                ctxt.variables.emplace_back(variable);
                ctxt.stack.emplace_back(&ctxt.variables.back());
                return true;
            }
        }

        // Is token an operator?
        {
            bool found = false;
            for (auto & f : ctxt.operators) {
                if (strcmp(f.name, token) == 0) {
                    _rpn_debug_callback(ctxt, "is operator");
                    if (rpn_stack_size(ctxt) < f.argc) {
                        char buffer[64];
                        sprintf(buffer, "%s: %u vs %u mismatch", f.name, rpn_stack_size(ctxt), f.argc);
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
        return (var.value.type == rpn_value::unknown);
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
