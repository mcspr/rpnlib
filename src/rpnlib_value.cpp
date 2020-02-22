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
#include "rpnlib_value.h"

#include <cstring>
#include <cstdlib>
#include <cmath>

// TODO: implement fs_math operations

rpn_value::rpn_value() :
    type(rpn_value::null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::string:
            new (&as_string) String(other.as_string);
            break;
        case rpn_value::boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::null:
        default:
            break;
    }
    type = other.type;
}

// TODO: global flag RPNLIB_STRING_IMPLEMENTATION, default String
//       implement `template <T> _rpn_value_destroy_string(rpn_value&) { ... }`
//       - ~basic_string() for std::string to test on host
//       - ~String() for Arduino String
rpn_value::rpn_value(rpn_value&& other) {
    switch (other.type) {
        case rpn_value::string: {
            new (&as_string) String(std::move(other.as_string));
            other.as_string.~String();
            break;
        }
        case rpn_value::boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::null:
        default:
            if (type == rpn_value::string) {
                as_string.~String();
            }
            break;
    }
    type = other.type;
    other.type = rpn_value::null;
}

rpn_value::rpn_value(bool value) :
    as_boolean(value),
    type(rpn_value::boolean)
{}

rpn_value::rpn_value(int32_t value) :
    as_f64(double(value)),
    type(rpn_value::f64)
{}

rpn_value::rpn_value(uint32_t value) :
    as_f64(double(value)),
    type(rpn_value::f64)
{}

rpn_value::rpn_value(double value) :
    as_f64(value),
    type(rpn_value::f64)
{}

rpn_value::rpn_value(const char* value) :
    type(rpn_value::string)
{
    new (&as_string) String(value);
}

rpn_value::rpn_value(const String& value) :
    type(rpn_value::string)
{
    new (&as_string) String(value);
}

rpn_value::rpn_value(String&& value) :
    type(rpn_value::string)
{
    new (&as_string) String(std::move(value));
}

rpn_value::~rpn_value() {
    if (type == rpn_value::string) {
        as_string.~String();
    }
}

rpn_value::operator bool() const {
    switch (type) {
        case rpn_value::boolean:
            return as_boolean;
        case rpn_value::f64:
            return (as_f64 != 0.0L) && !std::isnan(as_f64) && !std::isinf(as_f64);
        case rpn_value::string:
        default:
            return true;
    }
}

rpn_value::operator double() const {
    switch (type) {
        case rpn_value::f64:
            return as_f64;
        case rpn_value::boolean:
            return as_boolean ? 1.0L : 0.0L;
        default:
            return 0.0L;
    }
}

rpn_value::operator String() const {
    switch (type) {
        case rpn_value::string:
            return as_string;
        case rpn_value::f64: {
            return String(as_f64);
         }
        default:
            return "";
    }
}

bool rpn_value::operator <(const rpn_value& other) const {
    if ((type == other.type) && (type == rpn_value::f64)) {
        return (as_f64 < other.as_f64);
    }
    return false;
}

bool rpn_value::operator >(const rpn_value& other) const {
    if ((type == other.type) && (type == rpn_value::f64)) {
        return (as_f64 > other.as_f64);
    }
    return false;
}

bool rpn_value::operator ==(const rpn_value& other) const {
    if (type != other.type) {
        return false;
    }

    switch (type) {
        case rpn_value::string:
            return (as_string == other.as_string);
        case rpn_value::boolean:
            return (as_boolean == other.as_boolean);
        case rpn_value::f64:
            return (as_f64 == other.as_f64);
        default:
            return false;
    }
}

bool rpn_value::operator !=(const rpn_value& other) const {
    return not (*this == other);
}

bool rpn_value::operator >=(const rpn_value& other) const {
    return (*this == other) || (*this > other);
}

bool rpn_value::operator <=(const rpn_value& other) const {
    return (*this == other) || (*this < other);
}

rpn_value rpn_value::operator +(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if (type != other.type) {
        return val;
    }

    // just pass exact operation
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean + other.as_boolean;
    // concat strings
    } else if (type == rpn_value::string) {
        val.type = rpn_value::string;

        const auto our_size = as_string.length();
        const auto other_size = other.as_string.length();

        new (&val.as_string) String();
        val.as_string.reserve(our_size + other_size + 1);
        val.as_string += as_string;
        val.as_string += other.as_string;
    // do generic math
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 + other.as_f64;
    }

    return val;
}

rpn_value rpn_value::operator -(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if ((type != other.type) || (type == rpn_value::string) || (other.type == rpn_value::string)) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean - other.as_boolean;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 - other.as_f64;
    }

    return val;
}

rpn_value rpn_value::operator *(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if ((type != other.type) || (type == rpn_value::string) || (other.type == rpn_value::string)) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean * other.as_boolean;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 * other.as_f64;
    }

    return val;
}

rpn_value rpn_value::operator /(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if ((type != other.type) || (type == rpn_value::string) || (other.type == rpn_value::string)) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    if ((type == rpn_value::f64) && (other.as_f64 == 0)) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean / other.as_boolean;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 / other.as_f64;
    }

    return val;
}

rpn_value rpn_value::operator %(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if ((type != other.type) || (type == rpn_value::string) || (other.type == rpn_value::string)) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    if ((type == rpn_value::f64) && (other.as_f64 == 0)) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean % other.as_boolean;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 - (floor(as_f64 / other.as_f64) * other.as_f64);
    }

    return val;
}

rpn_value& rpn_value::operator =(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::string:
            if (type == rpn_value::string) {
                as_string = other.as_string;
            } else {
                new (&as_string) String(other.as_string);
            }
            break;
        case rpn_value::boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::null:
            if (type == rpn_value::string) {
                as_string.~String();
            }
        default:
            break;
    }
    type = other.type;
    return *this;
}

bool rpn_value::is(value_t value) const {
    return (type == value);
}

bool rpn_value::isNull() const {
    return is(rpn_value::null);
}

bool rpn_value::isBoolean() const {
    return is(rpn_value::boolean);
}

bool rpn_value::isString() const {
    return is(rpn_value::string);
}

bool rpn_value::isNumber() const {
    return is(rpn_value::f64);
}

