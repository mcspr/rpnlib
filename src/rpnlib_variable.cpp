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

#include "rpnlib.h"
#include "rpnlib_variable.h"

#include <algorithm>
#include <memory>

// ----------------------------------------------------------------------------
// Variables methods
// ----------------------------------------------------------------------------

size_t rpn_variables_size(rpn_context & ctxt) {
    return std::distance(ctxt.variables.begin(), ctxt.variables.end());
}

bool rpn_variables_clear(rpn_context & ctxt) {
    ctxt.variables.clear();
    return true;
}

bool rpn_variables_unref(rpn_context& ctxt) {
    ctxt.variables.remove_if([](const rpn_variable& var) {
        return (var.value.use_count() == 1) && (!static_cast<bool>(*var.value));
    });
    return true;
}

namespace {

template<typename Value>
bool _rpn_variable_set(rpn_context & ctxt, const std::string& name, Value&& value) {
    if (!name.length() || (name.find(' ') != std::string::npos)) {
        return false;
    }

    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        *v.value.get() = std::forward<Value>(value);
        return true;
    }

    ctxt.variables.emplace_front(name, std::make_shared<rpn_value>(std::forward<Value>(value)));
    return true;
}

}

bool rpn_variable_set(rpn_context & ctxt, const std::string& name, const rpn_value& value) {
    return _rpn_variable_set(ctxt, name, value);
}

bool rpn_variable_set(rpn_context & ctxt, const std::string& name, rpn_value&& value) {
    return _rpn_variable_set(ctxt, name, std::move(value));
}

bool rpn_variable_get(rpn_context & ctxt, const std::string& name, rpn_value& value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        value = *v.value.get();
        return true;
    }
    return false;
}

rpn_value rpn_variable_get(rpn_context & ctxt, const std::string& name) {
    rpn_value value;
    rpn_variable_get(ctxt, name, value);
    return value;
}

bool rpn_variable_del(rpn_context & ctxt, const std::string& name) {
    auto end = ctxt.variables.end();
    auto prev = ctxt.variables.before_begin();
    auto v = prev;

    while (v != end) {
        prev = v++;
        if ((*v).name == name) {
            ctxt.variables.erase_after(prev);
            return true;
        }
    }

    return false;
}

