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

#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <limits>

struct rpn_value {
    enum class Type {
        Null,
        Error,
        Boolean,
        Integer,
        Unsigned,
        Float,
        String
    };

    rpn_value();
    explicit rpn_value(rpn_value_error);
    explicit rpn_value(bool);
    explicit rpn_value(rpn_int);
    explicit rpn_value(rpn_uint);
    explicit rpn_value(rpn_float);
    explicit rpn_value(const char*);
    explicit rpn_value(const String&);
    explicit rpn_value(String&&);

    rpn_value(rpn_value&&) noexcept;
    rpn_value(const rpn_value&);
    ~rpn_value();

    void assign(const rpn_value&) noexcept;
    rpn_value& operator=(const rpn_value&);

    explicit operator bool() const;

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

    rpn_value_error toError() const;
    bool toBoolean() const;
    rpn_int toInt() const;
    rpn_uint toUint() const;
    rpn_float toFloat() const;
    String toString() const;

    bool is(Type) const;
    bool isError() const;
    bool isNull() const;
    bool isBoolean() const;
    bool isInt() const;
    bool isUint() const;
    bool isFloat() const;
    bool isNumber() const;
    bool isString() const;

    Type type;

    private:

    void assignPrimitive(const rpn_value&) noexcept;

    union {
        rpn_value_error as_error;
        bool as_boolean;
        rpn_int as_integer;
        rpn_uint as_unsigned;
        rpn_float as_float;
        String as_string;
    };

};

