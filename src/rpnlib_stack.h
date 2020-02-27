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
    RPN_STACK_TYPE_VALUE,
    RPN_STACK_TYPE_VARIABLE
};

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

size_t rpn_stack_size(rpn_context &);
bool rpn_stack_clear(rpn_context &);

bool rpn_stack_push(rpn_context &, float);
bool rpn_stack_push(rpn_context &, bool);
bool rpn_stack_push(rpn_context &, double);
bool rpn_stack_push(rpn_context &, const char*);
bool rpn_stack_push(rpn_context &, const String&);

bool rpn_stack_push(rpn_context &, const rpn_value &);
bool rpn_stack_push(rpn_context &, rpn_value &&);
bool rpn_stack_pop(rpn_context &, rpn_value &);

bool rpn_stack_pop(rpn_context &, float &);
bool rpn_stack_pop(rpn_context &, double &);
bool rpn_stack_pop(rpn_context &, bool &);
bool rpn_stack_pop(rpn_context &, String &);

bool rpn_stack_get(rpn_context &, unsigned char, float &);
bool rpn_stack_get(rpn_context &, unsigned char, double &);
bool rpn_stack_get(rpn_context &, unsigned char, bool &);
bool rpn_stack_get(rpn_context &, unsigned char, String &);

