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

#ifndef rpnlib_h
#define rpnlib_h

#include <Arduino.h>

#include "rpnlib_config.h"

#include <array>
#include <memory>
#include <vector>

enum class rpn_error_category {
    Unknown,
    Processing,
    Value
};

enum class rpn_value_error {
    OK,
    InvalidOperation,
    TypeMismatch,
    DivideByZero,
    IEEE754,
    IsNull
};

enum rpn_processing_error {
    RPN_ERROR_OK,
    RPN_ERROR_UNKNOWN_TOKEN,
    RPN_ERROR_ARGUMENT_COUNT_MISMATCH,
    RPN_ERROR_DIVIDE_BY_ZERO,
    RPN_ERROR_INVALID_OPERATION,
    RPN_ERROR_INVALID_ARGUMENT,
    RPN_ERROR_VARIABLE_DOES_NOT_EXIST,
    RPN_ERROR_STOP_PROCESSING
};

struct rpn_error {
    rpn_error();
    rpn_error(rpn_processing_error);
    rpn_error(rpn_value_error);

    rpn_error& operator =(rpn_processing_error);
    rpn_error& operator =(rpn_value_error);

    void reset();

    rpn_error_category category;
    int code;
};

using rpn_int_t = RPNLIB_INT_TYPE;
using rpn_float_t = RPNLIB_FLOAT_TYPE;
using rpn_uint_t = RPNLIB_UINT_TYPE;

struct rpn_variable;
struct rpn_operator;
struct rpn_stack_value;
struct rpn_context;

using rpn_debug_callback_f = void(*)(rpn_context &, const char *);

struct rpn_context {
    std::vector<rpn_variable> variables;
    std::vector<rpn_operator> operators;
    std::vector<rpn_stack_value> stack;

    rpn_debug_callback_f debug_callback;
    String input_buffer;
    rpn_error error;
};

#include <cstdint>

// ----------------------------------------------------------------------------

#include "rpnlib_value.h"
#include "rpnlib_operators.h"
#include "rpnlib_variable.h"
#include "rpnlib_stack.h"

// ----------------------------------------------------------------------------

bool rpn_process(rpn_context &, const char *, bool variable_must_exist = false);
bool rpn_init(rpn_context &);
bool rpn_clear(rpn_context &);

bool rpn_debug(rpn_context &, rpn_debug_callback_f);

// ----------------------------------------------------------------------------

#endif // rpnlib_h
