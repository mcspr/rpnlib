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

// TODO: handle assignment in rpn_value class method
// TODO: avoid exposing rpn_value::as_... members
bool rpn_variable_set(rpn_context & ctxt, const char * name, double value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        if (v.value->type != rpn_value::f64) break;
        v.value->as_f64 = value;
        return true;
    }

    ctxt.variables.emplace_back(name, std::make_shared<rpn_value>(value));
    return true;
}

bool rpn_variable_set(rpn_context & ctxt, const char * name, int value) {
    return rpn_variable_set(ctxt, name, double(value));
}

bool rpn_variable_set(rpn_context & ctxt, const char * name, long value) {
    return rpn_variable_set(ctxt, name, double(value));
}

bool rpn_variable_get(rpn_context & ctxt, const char * name, float & value) {
    for (auto& v : ctxt.variables) {
        if (v.name != name) continue;
        if (v.value->type != rpn_value::f64) break;
        value = v.value->as_f64;
        return true;
    }
    return false;
}

bool rpn_variable_del(rpn_context & ctxt, const char * name) {
    for (auto v = ctxt.variables.begin(); v != ctxt.variables.end(); ++v) {
        if ((*v).name == name) {
            ctxt.variables.erase(v);
            return true;
        }
    }
    return false;
}

size_t rpn_variables_size(rpn_context & ctxt) {
    return ctxt.variables.size();
}

const char * rpn_variable_name(rpn_context & ctxt, unsigned char i) {
    if (i < ctxt.variables.size()) {
        return ctxt.variables[i].name.c_str();
    }
    return NULL;
}

bool rpn_variables_clear(rpn_context & ctxt) {
    ctxt.variables.clear();
    return true;
}

