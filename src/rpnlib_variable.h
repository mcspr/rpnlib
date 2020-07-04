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

#pragma once

#include "rpnlib.h"
#include "rpnlib_value.h"

#include <cstdint>
#include <string>
#include <memory>
#include <type_traits>

struct rpn_variable {
    template <typename Name>
    rpn_variable(Name&& name, std::shared_ptr<rpn_value> value) :
        name(std::forward<Name>(name)),
        value(value)
    {}

    template <typename Name, typename Value>
    rpn_variable(Name&& name, Value&& value) :
        name(std::forward<Name>(name)),
        value(std::make_shared<rpn_value>(std::forward<Value>(value)))
    {}

    String name;
    std::shared_ptr<rpn_value> value;
};

template <typename Callback>
void rpn_variable_foreach(rpn_context & ctxt, Callback callback) {
    for (auto& variable : ctxt.variables) {
        callback(variable.name, *(variable.value.get()));
    }
}

bool rpn_variable_set(rpn_context &, const String& name, const rpn_value& value);
bool rpn_variable_set(rpn_context &, const String& name, rpn_value&& value);

bool rpn_variable_get(rpn_context &, const String& name, rpn_value& value);


bool rpn_variable_del(rpn_context &, const String& name);

size_t rpn_variables_size(rpn_context &);
bool rpn_variables_clear(rpn_context &);

bool rpn_variables_unref(rpn_context &);
