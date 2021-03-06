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
#include "rpnlib_variable.h"

// ----------------------------------------------------------------------------
// Stack methods
// ----------------------------------------------------------------------------

bool rpn_stack_push(rpn_context & ctxt, const rpn_value& value) {
    ctxt.stack.get().emplace_back(value);
    return true;
}

bool rpn_stack_push(rpn_context & ctxt, rpn_value&& value) {
    ctxt.stack.get().emplace_back(std::move(value));
    return true;
}

namespace {

// we only can manipulate the current stack
// stack shifting, push and pop is only handled via the [ and ] keywords
bool _rpn_stack_get(rpn_nested_stack::stack_type& stack, unsigned char index, rpn_value& out) {
    const auto size = stack.size();
    if (index >= size) return false;

    const auto& ref = stack.at(size - index - 1);
    if (ref.value == nullptr) return false;

    out = *ref.value.get();
    return true;
}

bool _rpn_stack_get(rpn_context & ctxt, unsigned char index, rpn_value& out) {
    auto& stack = ctxt.stack.get();
    return _rpn_stack_get(stack, index, out);
}

} // namespace

bool rpn_stack_get(rpn_context & ctxt, unsigned char index, rpn_value& out) {
    return _rpn_stack_get(ctxt, index, out);
}

rpn_value rpn_stack_pop(rpn_context & ctxt, unsigned char index) {
    rpn_value result;
    rpn_stack_get(ctxt, index, result);
    return result;
}

bool rpn_stack_pop(rpn_context & ctxt, rpn_value& out) {
    auto& stack = ctxt.stack.get();
    if (_rpn_stack_get(stack, 0, out)) {
        stack.pop_back();
        return true;
    }
    return false;
}

rpn_value rpn_stack_pop(rpn_context & ctxt) {
    rpn_value result;
    rpn_stack_pop(ctxt, result);
    return result;
}

size_t rpn_stack_size(rpn_context & ctxt) {
    return ctxt.stack.get().size();
}

bool rpn_stack_clear(rpn_context & ctxt) {
    ctxt.stack.stacks_clear();
    rpn_variables_unref(ctxt);
    return true;
}

rpn_stack_value::Type rpn_stack_inspect(rpn_context & ctxt) {
    auto& stack = ctxt.stack.get();
    if (stack.size()) {
        return stack.back().type;
    }

    return rpn_stack_value::Type::None;
}

void rpn_nested_stack::stacks_merge() {
    auto& prev = *(_stacks.end() - 2);
    auto& current = get();

    prev.insert(
        prev.end(),
        current.begin(),
        current.end()
    );
    prev.emplace_back(
        rpn_stack_value::Type::Array,
        rpn_value(static_cast<rpn_uint>(current.size()))
    );

    stacks_pop();
}
