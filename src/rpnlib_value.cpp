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

#include <limits>

#include <cstring>
#include <cstdlib>
#include <cmath>

// TODO: implement fs_math operations

namespace {

bool _rpn_is_zero(const rpn_value& value) {
    bool result = false;

    switch (value.type) {
        case rpn_value::f64:
            if (std::isinf(value.as_f64) || std::isnan(value.as_f64)) {
                break;
            }
            result = value.as_f64 == 0.0L;
            break;
        case rpn_value::i32:
            result = value.as_i32 == 0L;
            break;
        case rpn_value::u32:
            result = value.as_i32 == 0UL;
            break;
        default:
            break;
    }

    return result;
}

const char* _rpn_type(rpn_value::value_t type) {
    switch (type) {
        case rpn_value::null: return "null";
        case rpn_value::boolean: return "boolean";
        case rpn_value::i32: return "i32";
        case rpn_value::u32: return "u32";
        case rpn_value::f64: return "f64";
        case rpn_value::string: return "string";
    }
    return "(unknown)";
}

}

rpn_value::rpn_value() :
    type(rpn_value::null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::null:
            break;
        case rpn_value::boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::i32:
            as_i32 = other.as_i32;
            break;
        case rpn_value::u32:
            as_u32 = other.as_u32;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::string:
            new (&as_string) String(other.as_string);
            break;
        default:
            break;
    }
    type = other.type;
}

// TODO: global flag RPNLIB_STRING_IMPLEMENTATION, default String
//       implement `template <T> _rpn_value_destroy_string(rpn_value&) { ... }`
//       - ~basic_string() for std::string to test on host
//       - ~String() for Arduino String
rpn_value::rpn_value(rpn_value&& other) :
    type(rpn_value::null)
{
    assign(other);
    if (other.type == rpn_value::string) {
        other.as_string.~String();
    }
    other.type = rpn_value::null;
}

rpn_value::rpn_value(bool value) :
    as_boolean(value),
    type(rpn_value::boolean)
{}

rpn_value::rpn_value(int32_t value) :
    as_i32(value),
    type(rpn_value::i32)
{}

rpn_value::rpn_value(uint32_t value) :
    as_u32(value),
    type(rpn_value::u32)
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

void rpn_value::assign(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::null:
            if (type == rpn_value::string) {
                as_string.~String();
            }
        case rpn_value::boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::i32:
            as_i32 = other.as_i32;
            break;
        case rpn_value::u32:
            as_u32 = other.as_u32;
            break;
        case rpn_value::f64:
            as_f64 = other.as_f64;
            break;
        case rpn_value::string:
            if (type == rpn_value::string) {
                as_string = other.as_string;
            } else {
                new (&as_string) String(other.as_string);
            }
            break;
        default:
            break;
    }
    type = other.type;
}


rpn_value::operator bool() const {
    switch (type) {
        case rpn_value::boolean:
            return as_boolean;
        case rpn_value::i32:
        case rpn_value::u32:
        case rpn_value::f64:
            return !_rpn_is_zero(*this);
        case rpn_value::string:
            return as_string.length() > 0;
        default:
            return true;
    }
}

rpn_value::operator uint32_t() const {
    uint32_t result = 0UL;

    switch (type) {
        case rpn_value::u32:
            result = as_u32;
            break;
        case rpn_value::boolean:
            result = as_boolean ? 1UL : 0UL;
            break;
        case rpn_value::i32:
            if (as_i32 >= 0) {
                result = as_i32;
            }
            break;
        case rpn_value::f64:
            if ((std::numeric_limits<uint32_t>::min() <= as_f64)
                && (std::numeric_limits<uint32_t>::max() < as_f64)) {
                result = static_cast<uint32_t>(as_f64);
            }
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator double() const {
    double result = 0.0L;

    switch (type) {
        case rpn_value::f64:
            result = as_f64;
            break;
        case rpn_value::boolean:
            result = as_boolean ? 1.0L : 0.0L;
            break;
        case rpn_value::i32:
            result = as_i32;
            break;
        case rpn_value::u32:
            result = as_u32;
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator int32_t() const {
    int32_t result = 0;

    switch (type) {
        case rpn_value::i32:
            result = as_i32;
            break;
        case rpn_value::boolean:
            result = as_boolean ? 1.0L : 0.0L;
            break;
        case rpn_value::u32:
            result = as_u32;
            break;
        case rpn_value::f64:
            if ((std::numeric_limits<int32_t>::min() <= as_f64)
                && (std::numeric_limits<int32_t>::max() < as_f64)) {
                result = lround(as_f64);
            }
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator String() const {
    String result("");

    switch (type) {
        case rpn_value::string:
            return as_string;
        case rpn_value::i32:
            result = String(as_i32);
        case rpn_value::u32:
            result = String(as_u32);
        case rpn_value::f64:
            result = String(as_f64);
            break;
        default:
            break;
    }

    return result;
}

bool rpn_value::operator <(const rpn_value& other) const {
    bool result = false;

    switch (type) {
        case rpn_value::f64:
            result = as_f64 < double(other);
            break;
        case rpn_value::i32:
            result = as_i32 < int32_t(other);
            break;
        case rpn_value::u32:
            result = as_u32 < uint32_t(other);
            break;
        case rpn_value::string:
        case rpn_value::boolean:
        default:
            break;
    }

    return result;
}

bool rpn_value::operator >(const rpn_value& other) const {
    bool result = false;

    switch (type) {
        case rpn_value::f64:
            result = as_f64 > double(other);
            break;
        case rpn_value::i32:
            result = as_i32 > int32_t(other);
            break;
        case rpn_value::u32:
            result = as_u32 > uint32_t(other);
            break;
        case rpn_value::string:
        case rpn_value::boolean:
        default:
            break;
    }

    return result;
}

bool rpn_value::operator ==(const rpn_value& other) const {
    bool result = false;

    switch (type) {
        case rpn_value::string:
            result = (as_string == other.as_string);
            break;
        case rpn_value::boolean:
            result = as_boolean == bool(other);
            break;
        case rpn_value::f64:
            result = as_f64 == double(other);
            break;
        case rpn_value::i32:
            result = as_i32 == int32_t(other);
            break;
        case rpn_value::u32:
            result = as_u32 == uint32_t(other);
            break;
        default:
            break;
    }

    return result;
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

    switch (type) {
        // concat strings
        case rpn_value::string: {
            if (other.type != rpn_value::string) {
                break;
            }
            val.type = rpn_value::string;

            const auto our_size = as_string.length();
            const auto other_size = other.as_string.length();

            new (&val.as_string) String();
            val.as_string.reserve(our_size + other_size + 1);
            val.as_string += as_string;
            val.as_string += other.as_string;
            break;
        }
        // just do generic math
        case rpn_value::boolean:
            if (other.type != rpn_value::boolean) {
                break;
            }
            val.type = rpn_value::boolean;
            val.as_boolean = as_boolean + other.as_boolean;
            break;
        case rpn_value::f64:
            val.type = rpn_value::f64;
            val.as_f64 = as_f64 + double(other);
            break;
        case rpn_value::i32:
            val.type = rpn_value::i32;
            val.as_i32 = as_i32 + int32_t(other);
            break;
        case rpn_value::u32:
            val.type = rpn_value::u32;
            val.as_u32 = as_u32 + uint32_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator -(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::boolean:
            val.type = rpn_value::boolean;
            val.as_boolean = as_boolean - bool(other);
            break;
        case rpn_value::f64:
            val.type = rpn_value::f64;
            val.as_f64 = as_f64 - double(other);
            break;
        case rpn_value::i32:
            val.type = rpn_value::i32;
            val.as_i32 = as_i32 - int32_t(other);
            break;
        case rpn_value::u32:
            val.type = rpn_value::u32;
            val.as_u32 = as_u32 - uint32_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator *(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // jost do generic math
    switch (type) {
        case rpn_value::boolean:
            val.type = rpn_value::boolean;
            val.as_boolean = as_boolean && bool(other);
            break;
        case rpn_value::f64:
            val.type = rpn_value::f64;
            val.as_f64 = as_f64 * double(other);
            break;
        case rpn_value::i32:
            val.type = rpn_value::i32;
            val.as_i32 = as_i32 * int32_t(other);
            break;
        case rpn_value::u32:
            val.type = rpn_value::u32;
            val.as_u32 = as_u32 * uint32_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator /(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    if (_rpn_is_zero(other)) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::boolean:
            val.type = rpn_value::boolean;
            val.as_boolean = as_boolean / bool(other);
            break;
        case rpn_value::f64:
            val.type = rpn_value::f64;
            val.as_f64 = as_f64 / double(other);
            break;
        case rpn_value::i32:
            val.type = rpn_value::i32;
            val.as_i32 = as_i32 / int32_t(other);
            break;
        case rpn_value::u32:
            val.type = rpn_value::u32;
            val.as_u32 = as_u32 / uint32_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator %(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    if (_rpn_is_zero(other)) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::boolean:
            val.type = rpn_value::boolean;
            val.as_boolean = as_boolean % other.as_boolean;
            break;
        case rpn_value::i32:
            val.type = rpn_value::i32;
            val.as_i32 = as_i32 - (floor(as_i32 / double(other.as_i32)) * other.as_i32);
            break;
        case rpn_value::u32:
            val.type = rpn_value::u32;
            val.as_u32 = as_u32 - (floor(as_u32 / double(other.as_u32)) * other.as_u32);
            break;
        case rpn_value::f64:
            val.type = rpn_value::f64;
            val.as_f64 = as_f64 - (floor(as_f64 / other.as_f64) * other.as_f64);
            break;
        default:
            break;
    }

    return val;
}

rpn_value& rpn_value::operator =(const rpn_value& other) {
    assign(other);
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
    return is(rpn_value::f64) || is(rpn_value::i32) || is(rpn_value::u32);
}

bool rpn_value::isFloat() const {
    return is(rpn_value::f64);
}

bool rpn_value::isInt() const {
    return is(rpn_value::i32);
}

bool rpn_value::isUint() const {
    return is(rpn_value::u32);
}

