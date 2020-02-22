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

#include <cstdint>
#include <cstring>

struct rpn_value {
    enum value_t {
        null,
        boolean,
        string,
        f64
    };

    rpn_value();
    rpn_value(bool);
    rpn_value(int32_t);
    rpn_value(uint32_t);
    rpn_value(double);
    rpn_value(const char*);
    rpn_value(const String&);
    rpn_value(String&&);

    rpn_value(rpn_value&&);
    rpn_value(const rpn_value&);
    ~rpn_value();

    rpn_value& operator=(const rpn_value&);

    bool operator >(const rpn_value&) const;
    bool operator <(const rpn_value&) const;
    bool operator ==(const rpn_value&) const;
    bool operator !=(const rpn_value&) const;

    bool operator >=(const rpn_value&) const;
    bool operator <=(const rpn_value&) const;

    rpn_value operator +(const rpn_value&);
    rpn_value operator -(const rpn_value&);
    rpn_value operator *(const rpn_value&);
    rpn_value operator /(const rpn_value&);
    rpn_value operator %(const rpn_value&);

    operator bool() const;
    operator double() const;
    operator String() const;

    bool is(value_t) const;
    bool isNull() const;
    bool isBoolean() const;
    bool isString() const;
    bool isNumber() const;

    union {
        bool as_boolean;
        double as_f64;
        String as_string;
    };

    value_t type;

};

