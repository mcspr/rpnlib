/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

// ----------------------------------------------------------------------------
// Internal struct implementation
// ----------------------------------------------------------------------------

rpn_stack_value::rpn_stack_value(rpn_stack_type_t type, std::shared_ptr<rpn_value> value) :
    type(type),
    value(value)
{}

rpn_stack_value::rpn_stack_value(std::shared_ptr<rpn_value> value) :
    rpn_stack_value(RPN_STACK_TYPE_VALUE, value)
{}

rpn_stack_value::rpn_stack_value(rpn_stack_type_t type, const rpn_value& value) :
    rpn_stack_value(type, std::make_shared<rpn_value>(value))
{}

rpn_stack_value::rpn_stack_value(rpn_stack_type_t type, rpn_value&& value) :
    rpn_stack_value(type, std::make_shared<rpn_value>(std::move(value)))
{}

rpn_stack_value::rpn_stack_value(const rpn_value& value) :
    rpn_stack_value(RPN_STACK_TYPE_VALUE, std::make_shared<rpn_value>(value))
{}

rpn_stack_value::rpn_stack_value(rpn_value&& value) :
    rpn_stack_value(RPN_STACK_TYPE_VALUE, std::make_shared<rpn_value>(std::move(value)))
{}

// ----------------------------------------------------------------------------
// Stack methods
// ----------------------------------------------------------------------------

size_t rpn_stack_size(rpn_context & ctxt) {
    return ctxt.stack.size();
}

bool rpn_stack_clear(rpn_context & ctxt) {
    ctxt.stack.clear();
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, const rpn_value& value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, rpn_value&& value) {
    ctxt.stack.emplace_back(std::move(value));
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, bool value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, double value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, int32_t value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, uint32_t value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, char* value) {
    ctxt.stack.emplace_back(value);
    return true;
}

bool rpn_stack_get(rpn_context & ctxt, unsigned char index, double & value) {
    const auto size = ctxt.stack.size();
    if (index >= size) return false;

    const auto& ref = ctxt.stack.at(size - index - 1);
    if (!ref.value) return false;

    value = double(*ref.value.get());

    return true;
}

bool rpn_stack_pop(rpn_context & ctxt, double & value) {
    if (!ctxt.stack.size()) return false;

    const auto& ref = ctxt.stack.back();
    if (!ref.value) return false;

    value = double(*ref.value.get());

    ctxt.stack.pop_back();
    return true;
}

