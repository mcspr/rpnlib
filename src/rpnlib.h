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
#include "rpnlib_error.h"

#include <vector>

using rpn_int = RPNLIB_INT_TYPE;
using rpn_float = RPNLIB_FLOAT_TYPE;
using rpn_uint = RPNLIB_UINT_TYPE;

struct rpn_context;

// ----------------------------------------------------------------------------

#include "rpnlib_value.h"
#include "rpnlib_operators.h"
#include "rpnlib_variable.h"
#include "rpnlib_stack.h"

struct rpn_context {
    using debug_callback_type = void(*)(rpn_context &, const char *);
    using operators_type = std::vector<rpn_operator>;
    using variables_type = std::vector<rpn_variable>;

    debug_callback_type debug_callback;

    String input_buffer;
    rpn_error error;

    variables_type variables;
    operators_type operators;
    rpn_nested_stack stack;
};

template <typename Callback>
void rpn_stack_foreach(rpn_context & ctxt, Callback callback) {
    auto& stack = ctxt.stack.get();
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        callback((*it).type, *((*it).value.get()));
    }
}

template <typename Callback>
void rpn_variables_foreach(rpn_context & ctxt, Callback callback) {
    for (auto& var : ctxt.variables) {
        callback(var.name, *(var.value.get()));
    }
}

template <typename Callback>
void rpn_operators_foreach(rpn_context & ctxt, Callback callback) {
    for (auto& op : ctxt.operators) {
        callback(op.name, op.argc, op.callback);
    }
}

// ----------------------------------------------------------------------------

bool rpn_process(rpn_context &, const char *, bool variable_must_exist = false);
bool rpn_init(rpn_context &);
bool rpn_clear(rpn_context &);

bool rpn_debug(rpn_context &, rpn_context::debug_callback_type);

// ----------------------------------------------------------------------------

#endif // rpnlib_h
