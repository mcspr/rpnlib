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

struct rpn_variable {
    rpn_variable(const rpn_variable& other);
    rpn_variable(rpn_variable&& other);

    rpn_variable(const String& name);
    rpn_variable(const char* name);

    rpn_variable(const String& name, std::shared_ptr<rpn_value> value);
    rpn_variable(const char* name, std::shared_ptr<rpn_value> value);

    rpn_variable(const char* name, const rpn_value& value);
    rpn_variable(const char* name, rpn_value&& value);

    rpn_variable& operator =(const rpn_variable& other);
    rpn_variable& operator =(rpn_variable&& other);

    String name;
    std::shared_ptr<rpn_value> value;
};

bool rpn_variable_set(rpn_context &, const char *, bool);
bool rpn_variable_get(rpn_context &, const char *, bool &);

bool rpn_variable_set(rpn_context &, const char *, double);
bool rpn_variable_get(rpn_context &, const char *, double &);

bool rpn_variable_set(rpn_context &, const char *, int32_t);
bool rpn_variable_get(rpn_context &, const char *, int32_t &);

bool rpn_variable_set(rpn_context &, const char *, char *);
bool rpn_variable_get(rpn_context &, const char *, char **);

bool rpn_variable_del(rpn_context &, const char *);
size_t rpn_variables_size(rpn_context &);
const char * rpn_variable_name(rpn_context &, unsigned char);
bool rpn_variables_clear(rpn_context &);

