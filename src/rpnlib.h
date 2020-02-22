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

#ifndef rpnlib_h
#define rpnlib_h

// ----------------------------------------------------------------------------

#include <vector>
#include <cstdint>

// ----------------------------------------------------------------------------

struct rpn_value {
    enum value_t {
        unknown,
        s32,
        u32,
        f64,
        charptr
    };

    rpn_value();
    rpn_value(int32_t);
    rpn_value(uint32_t);
    rpn_value(double);
    rpn_value(char*);

    rpn_value(rpn_value&&);
    rpn_value(const rpn_value&);
    ~rpn_value();

    rpn_value& operator=(const rpn_value&);

    operator int32_t() const;
    operator uint32_t() const;
    operator double() const;
    operator char*() const;

    // TODO: generic variant struct to manage String / std::string / custom string obj member
    // TODO: if not, sso?
    union {
        int32_t as_s32;
        uint32_t as_u32;
        double as_f64;
        char* as_charptr;
    };

    value_t type;
};

struct rpn_variable {
    rpn_variable() : name(nullptr) {}
    ~rpn_variable(); 
    rpn_variable(const rpn_variable&);
    rpn_variable(rpn_variable&&);
    rpn_variable& operator=(const rpn_variable&);
    char * name; // TODO: use String / std::string / custom string obj
    rpn_value value; // TODO: track value with shared_ptr to allow to share it with stack-value
};

struct rpn_stack_value {
    rpn_stack_value(const rpn_value&, rpn_variable* variable);
    rpn_stack_value(const rpn_value&);
    rpn_stack_value(rpn_stack_value&& other) {
        variable = other.variable;
        value = other.value;
        other.variable = nullptr;
    }
    rpn_stack_value& operator=(const rpn_stack_value& other) {
        variable = other.variable;
        value = other.value;
        return *this;
    }
    rpn_stack_value(rpn_variable*);
    rpn_variable* variable; // TODO: track variable with shared_ptr to avoid outdated ptr
    rpn_value value; // TODO: track value with shared_ptr to allow to share it with variable
};

struct rpn_context;

struct rpn_operator {
    char * name; // TODO: use String / std::string / custom string obj 
    unsigned char argc;
    bool (*callback)(rpn_context &);
};

struct rpn_context {
    std::vector<rpn_stack_value> stack;
    std::vector<rpn_variable> variables;
    std::vector<rpn_operator> operators;
};

enum rpn_errors {
    RPN_ERROR_OK,
    RPN_ERROR_UNKNOWN_TOKEN,
    RPN_ERROR_ARGUMENT_COUNT_MISMATCH,
    RPN_ERROR_DIVIDE_BY_ZERO,
    RPN_ERROR_UNVALID_ARGUMENT,
    RPN_ERROR_VARIABLE_DOES_NOT_EXIST
};

using rpn_debug_callback_f = void(*)(rpn_context &, const char *);

// ----------------------------------------------------------------------------

extern rpn_errors rpn_error;
extern rpn_debug_callback_f _rpn_debug_callback;

// ----------------------------------------------------------------------------

bool rpn_operators_init(rpn_context &);
bool rpn_operator_set(rpn_context &, const char *, unsigned char, bool (*)(rpn_context &));
bool rpn_operators_clear(rpn_context &);

bool rpn_variable_set(rpn_context &, const char *, float);
bool rpn_variable_get(rpn_context &, const char *, float &);
bool rpn_variable_del(rpn_context &, const char *);
unsigned char rpn_variables_size(rpn_context &);
char * rpn_variable_name(rpn_context &, unsigned char);
bool rpn_variables_clear(rpn_context &);

bool rpn_stack_clear(rpn_context &);
bool rpn_stack_push(rpn_context &, float);
bool rpn_stack_pop(rpn_context &, float &);
unsigned char rpn_stack_size(rpn_context &);
bool rpn_stack_get(rpn_context &, unsigned char, float &);

bool rpn_process(rpn_context &, const char *, bool variable_must_exist = false);
bool rpn_init(rpn_context &);
bool rpn_clear(rpn_context &);

bool rpn_debug(rpn_debug_callback_f);

// ----------------------------------------------------------------------------

#endif // rpnlib_h
