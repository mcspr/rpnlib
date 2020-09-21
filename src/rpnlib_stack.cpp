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
bool _rpn_stack_get(rpn_context & ctxt, unsigned char index, rpn_value& out) {
    auto& stack = ctxt.stack.get();

    const auto size = stack.size();
    if (index >= size) return false;

    const auto& ref = stack.at(size - index - 1);
    if (!ref.value) {
        return false;
    }

    out = *(ref.value.get());
    return true;
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
    if (_rpn_stack_get(ctxt, 0, out)) {
        ctxt.stack.pop();
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
    return ctxt.stack.size();
}

bool rpn_stack_clear(rpn_context & ctxt) {
    ctxt.stack.stacks_clear();
    rpn_variables_unref(ctxt);
    return true;
}

rpn_stack_value::Type rpn_stack_inspect(rpn_context & ctxt) {
    if (!ctxt.stack.size()) return rpn_stack_value::Type::None;
    return ctxt.stack.back().type;
}

bool rpn_nested_stack::stacks_merge(Reason reason) {
    if ((reason != Reason::None) && (reason == _stacks.back().reason)) {
        auto& prev = *(_stacks.end() - 2);
        prev.stack.reserve(prev.stack.size() + _current->stack.size());

        auto block = stacks_size();
        for (auto&& current : _current->stack) {
            if (current.block < block) {
                current.block = block;
            }
            prev.stack.push_back(std::move(current));
        }

        auto type = (Reason::Array == reason) ? rpn_stack_value::Type::Array :
                    (Reason::Block == reason) ? rpn_stack_value::Type::Block :
                    rpn_stack_value::Type::None;

        if (rpn_stack_value::Type::None != type) {
            prev.stack.emplace_back(
                block,
                type,
                rpn_value(static_cast<rpn_uint>(_current->stack.size()))
            );
        }

        stacks_pop();

        return true;
    }

    return false;
}
