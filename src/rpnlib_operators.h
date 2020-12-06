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

struct rpn_operator {
    using callback_type = rpn_error(*)(rpn_context &);

    rpn_operator() = delete;

    template <typename T>
    rpn_operator(T&& name_, unsigned char argc_, callback_type callback_) :
        name(std::forward<T>(name_)),
        argc(argc_),
        callback(callback_)
    {}

    rpn_operator(const rpn_operator&) = default;
    rpn_operator(rpn_operator&& other) noexcept :
        name(std::move(other.name)),
        argc(other.argc),
        callback(other.callback)
    {}

    std::string name;
    unsigned char argc;
    callback_type callback;
};

bool rpn_operators_fmath_init(rpn_context &);
bool rpn_operators_init(rpn_context &);

bool rpn_operators_clear(rpn_context &);

bool rpn_operator_set(rpn_context &, const char *, unsigned char, rpn_operator::callback_type);
