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

#include <limits>

struct rpn_value {
    enum class Type {
        Null,
        Boolean,
        Integer,
        Unsigned,
        Float,
        String
    };

    rpn_value();
    explicit rpn_value(bool);
    explicit rpn_value(rpn_int_t);
    explicit rpn_value(rpn_uint_t);
    explicit rpn_value(rpn_float_t);
    explicit rpn_value(const char*);
    explicit rpn_value(const String&);
    explicit rpn_value(String&&);

    rpn_value(rpn_value&&);
    rpn_value(const rpn_value&);
    ~rpn_value();

    void assign(const rpn_value&);
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

    explicit operator bool() const;
    explicit operator rpn_int_t() const;
    explicit operator rpn_uint_t() const;
    explicit operator rpn_float_t() const;
    explicit operator String() const;

    bool is(Type) const;
    bool isNull() const;
    bool isBoolean() const;
    bool isInt() const;
    bool isUint() const;
    bool isFloat() const;
    bool isNumber() const;
    bool isString() const;

    private:

    union {
        bool as_boolean;
        rpn_int_t as_integer;
        rpn_uint_t as_unsigned;
        rpn_float_t as_float;
        String as_string;
    };

    public:

    Type type;

};

