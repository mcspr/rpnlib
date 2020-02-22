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

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// ----------------------------------------------------------------------------

struct rpn_value {
    enum value_t {
        null,
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
    std::string name;
    std::shared_ptr<rpn_value> value;
};

struct rpn_stack_value {
    rpn_stack_value(double value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_stack_value(int32_t value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_stack_value(uint32_t value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_stack_value(char* value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_stack_value(std::shared_ptr<rpn_variable> variable) :
        variable(variable),
        value(variable->value)
    {}

    rpn_stack_value(std::shared_ptr<rpn_value> value) :
        variable(nullptr),
        value(value)
    {}

    std::shared_ptr<rpn_variable> variable;
    std::shared_ptr<rpn_value> value;
};

struct rpn_context;

struct rpn_operator {
    std::string name;
    unsigned char argc;
    bool (*callback)(rpn_context &);
};

struct rpn_context {
    std::vector<std::shared_ptr<rpn_variable>> variables;
    std::vector<rpn_operator> operators;
    std::vector<rpn_stack_value> stack;
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
const char * rpn_variable_name(rpn_context &, unsigned char);
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
