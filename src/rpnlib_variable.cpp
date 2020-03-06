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

#include <memory>

// ----------------------------------------------------------------------------
// Internal struct implementation
// ----------------------------------------------------------------------------

rpn_variable::rpn_variable(const rpn_variable& other) :
    name(other.name),
    value(other.value)
{}

rpn_variable::rpn_variable(rpn_variable&& other) :
    name(std::move(other.name)),
    value(other.value)
{}

rpn_variable& rpn_variable::operator =(const rpn_variable& other) {
    name = other.name;
    value = other.value;
    return *this;
}

rpn_variable& rpn_variable::operator =(rpn_variable&& other) {
    name = std::move(other.name);
    value = std::move(other.value);
    return *this;
}

rpn_variable::rpn_variable(const String& name, std::shared_ptr<rpn_value> value) :
    name(name),
    value(value)
{}

rpn_variable::rpn_variable(const char* name, std::shared_ptr<rpn_value> value) :
    name(name),
    value(value)
{}

rpn_variable::rpn_variable(const String& name) :
    rpn_variable(name, std::make_shared<rpn_value>())
{}

rpn_variable::rpn_variable(const char* name) :
    rpn_variable(name, std::make_shared<rpn_value>())
{}

rpn_variable::rpn_variable(const char* name, const rpn_value& value) :
    rpn_variable(name, std::make_shared<rpn_value>(value))
{}

rpn_variable::rpn_variable(const char* name, rpn_value&& value) :
    rpn_variable(name, std::make_shared<rpn_value>(std::move(value)))
{}

// ----------------------------------------------------------------------------
// Variables methods
// ----------------------------------------------------------------------------

size_t rpn_variables_size(rpn_context & ctxt) {
    return ctxt.variables.size();
}

const char * rpn_variable_name(rpn_context & ctxt, unsigned char i) {
    if (i < ctxt.variables.size()) {
        return ctxt.variables[i].name.c_str();
    }
    return nullptr;
}

bool rpn_variables_clear(rpn_context & ctxt) {
    ctxt.variables.clear();
    return true;
}

namespace {

template<typename T>
bool _rpn_variable_set(rpn_context & ctxt, const char * name, T&& value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        *v.value.get() = std::forward<T>(value);
        return true;
    }

    ctxt.variables.emplace_back(name, std::make_shared<rpn_value>(std::forward<T>(value)));
    return true;
}

}

bool rpn_variable_set(rpn_context & ctxt, const char * name, const rpn_value& value) {
    return _rpn_variable_set(ctxt, name, value);
}

bool rpn_variable_set(rpn_context & ctxt, const char * name, rpn_value&& value) {
    return _rpn_variable_set(ctxt, name, std::move(value));
}

bool rpn_variable_get(rpn_context & ctxt, const char * name, rpn_value& value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        value = *v.value.get();
        return true;
    }
    return false;
}

template<typename T>
bool rpn_variable_get(rpn_context & ctxt, const char * name, T& value) {
    rpn_value tmp;
    if (rpn_variable_get(ctxt, name, tmp)) {
        value = tmp;
        return true;
    }
    return false;
}

template
bool rpn_variable_get<bool>(rpn_context & ctxt, const char * name, bool& value);

template
bool rpn_variable_get<rpn_float_t>(rpn_context & ctxt, const char * name, rpn_float_t& value);

template
bool rpn_variable_get<String>(rpn_context & ctxt, const char * name, String& value);

template
bool rpn_variable_get<rpn_int_t>(rpn_context & ctxt, const char * name, rpn_int_t& value);

template
bool rpn_variable_get<rpn_uint_t>(rpn_context & ctxt, const char * name, rpn_uint_t& value);

bool rpn_variable_del(rpn_context & ctxt, const char * name) {
    for (auto v = ctxt.variables.begin(); v != ctxt.variables.end(); ++v) {
        if ((*v).name == name) {
            ctxt.variables.erase(v);
            return true;
        }
    }
    return false;
}

