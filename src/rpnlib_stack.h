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

#pragma once

#include "rpnlib.h"
#include "rpnlib_value.h"

#include <memory>

struct rpn_context;

enum rpn_stack_type_t {
    RPN_STACK_TYPE_NONE,
    RPN_STACK_TYPE_VALUE,
    RPN_STACK_TYPE_VARIABLE
};

// TODO: 0.5.0 direct class methods instead of c style functions
//       return this struct as 'optional' type instead of bool
struct rpn_stack_value {

    rpn_stack_value(rpn_stack_type_t type, std::shared_ptr<rpn_value> value);
    rpn_stack_value(std::shared_ptr<rpn_value> value);

    rpn_stack_value(rpn_stack_type_t type, const rpn_value& value);
    rpn_stack_value(rpn_stack_type_t type, rpn_value&& value);

    rpn_stack_value(const rpn_value& value);
    rpn_stack_value(rpn_value&& value);

    rpn_stack_type_t type;
    std::shared_ptr<rpn_value> value;

};

rpn_stack_type_t rpn_stack_inspect(rpn_context & ctxt);

size_t rpn_stack_size(rpn_context &);
bool rpn_stack_clear(rpn_context &);

template<typename T>
inline bool rpn_stack_push(rpn_context & ctxt, T&& value) {
    ctxt.stack.emplace_back(rpn_value(std::forward<T>(value)));
    return ctxt.stack.size();
}

template<>
inline bool rpn_stack_push(rpn_context & ctxt, const rpn_value& value) {
    ctxt.stack.emplace_back(value);
    return true;
}

template<>
inline bool rpn_stack_push(rpn_context & ctxt, rpn_value&& value) {
    ctxt.stack.emplace_back(std::move(value));
    return true;
}

bool rpn_stack_get(rpn_context & ctxt, unsigned char index, rpn_value& value);

template <typename T>
inline bool rpn_stack_get(rpn_context & ctxt, unsigned char index, T& value) {
    rpn_value tmp;
    if (rpn_stack_get(ctxt, index, tmp)) {
        value = T(tmp);
        return true;
    }
    return false;
}

bool rpn_stack_pop(rpn_context & ctxt, rpn_value& value);

template<typename T>
bool rpn_stack_pop(rpn_context & ctxt, T& value) {
    rpn_value tmp;
    if (rpn_stack_pop(ctxt, tmp)) {
        value = T(tmp);
        return true;
    }
    return false;
}
