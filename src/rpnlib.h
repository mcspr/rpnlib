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

#include "rpnlib_value.h"

// ----------------------------------------------------------------------------

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// ----------------------------------------------------------------------------

struct rpn_variable {
    std::string name;
    std::shared_ptr<rpn_value> value;

    rpn_variable(const std::string& name) :
        name(name),
        value(std::make_shared<rpn_value>())
    {}

    rpn_variable(const char* name) :
        name(name),
        value(std::make_shared<rpn_value>())
    {}

    rpn_variable(const char* name, const rpn_value& value) :
        name(name),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_variable(const char* name, rpn_value&& value) :
        name(name),
        value(std::make_shared<rpn_value>(std::move(value)))
    {}

    rpn_variable(const char* name, std::shared_ptr<rpn_value> value) :
        name(name),
        value(value)
    {}

};

struct rpn_stack_value {
    rpn_stack_value(bool value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

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

    rpn_stack_value(const rpn_value& value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(value))
    {}

    rpn_stack_value(rpn_value&& value) :
        variable(nullptr),
        value(std::make_shared<rpn_value>(std::move(value)))
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
    RPN_ERROR_VARIABLE_DOES_NOT_EXIST,
    RPN_ERROR_TYPE_MISMATCH
};

enum rpn_token_t {
    RPN_TOKEN_UNKNOWN,
    RPN_TOKEN_WORD,
    RPN_TOKEN_NUMBER,
    RPN_TOKEN_STRING,
    RPN_TOKEN_VARIABLE,
};

using rpn_debug_callback_f = void(*)(rpn_context &, const char *);

// ----------------------------------------------------------------------------

extern rpn_errors rpn_error;
extern rpn_debug_callback_f _rpn_debug_callback;

// ----------------------------------------------------------------------------

bool rpn_operators_init(rpn_context &);
bool rpn_operator_set(rpn_context &, const char *, unsigned char, bool (*)(rpn_context &));
bool rpn_operators_clear(rpn_context &);

bool rpn_variable_set(rpn_context &, const char *, bool);
bool rpn_variable_get(rpn_context &, const char *, bool &);

bool rpn_variable_set(rpn_context &, const char *, double);
bool rpn_variable_get(rpn_context &, const char *, double &);

bool rpn_variable_set(rpn_context &, const char *, int32_t);
bool rpn_variable_get(rpn_context &, const char *, int32_t &);

bool rpn_variable_set(rpn_context &, const char *, uint32_t);
bool rpn_variable_get(rpn_context &, const char *, uint32_t &);

bool rpn_variable_set(rpn_context &, const char *, char *);
bool rpn_variable_get(rpn_context &, const char *, char **);

bool rpn_variable_del(rpn_context &, const char *);
size_t rpn_variables_size(rpn_context &);
const char * rpn_variable_name(rpn_context &, unsigned char);
bool rpn_variables_clear(rpn_context &);

bool rpn_stack_clear(rpn_context &);

bool rpn_stack_push(rpn_context &, float);
bool rpn_stack_push(rpn_context &, bool);
bool rpn_stack_push(rpn_context &, double);
bool rpn_stack_push(rpn_context &, int32_t);
bool rpn_stack_push(rpn_context &, uint32_t);
bool rpn_stack_push(rpn_context &, char*);

bool rpn_stack_push(rpn_context &, const rpn_value &);
bool rpn_stack_push(rpn_context &, rpn_value &&);
bool rpn_stack_pop(rpn_context &, rpn_value &);

bool rpn_stack_pop(rpn_context &, float &);

size_t rpn_stack_size(rpn_context &);
bool rpn_stack_get(rpn_context &, unsigned char, float &);

bool rpn_process(rpn_context &, const char *, bool variable_must_exist = false);
bool rpn_init(rpn_context &);
bool rpn_clear(rpn_context &);

bool rpn_debug(rpn_debug_callback_f);

// ----------------------------------------------------------------------------

#endif // rpnlib_h
