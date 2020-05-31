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

bool _rpn_can_divide_by(const rpn_value& value) {
    bool result = false;

    switch (value.type) {
        case rpn_value::Type::Float: {
            auto as_float = rpn_float_t(value);
            if (std::isinf(as_float) || std::isnan(as_float)) {
                rpn_error = RPN_ERROR_IEEE_754;
                return result;
            }
            result = as_float != 0.0L;
            break;
        }
        case rpn_value::Type::Null:
            rpn_error = RPN_ERROR_VALUE_IS_NULL;
            return result;
        case rpn_value::Type::Integer:
            result = rpn_int_t(value) != 0L;
            break;
        case rpn_value::Type::Unsigned:
            result = rpn_int_t(value) != 0UL;
            break;
        default:
            break;
    }

    if (!result) {
        rpn_error = RPN_ERROR_DIVIDE_BY_ZERO;
    }

    return result;
}

const char* _rpn_type(rpn_value::Type type) {
    switch (type) {
        case rpn_value::Type::Null: return "Null";
        case rpn_value::Type::Boolean: return "Boolean";
        case rpn_value::Type::Integer: return "Integer";
        case rpn_value::Type::Unsigned: return "Unsigned";
        case rpn_value::Type::Float: return "Float";
        case rpn_value::Type::String: return "String";
    }
    return "(unknown)";
}

}

rpn_value::rpn_value() :
    type(rpn_value::Type::Null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::Type::Null:
            break;
        case rpn_value::Type::Boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::Type::Integer:
            as_integer = other.as_integer;
            break;
        case rpn_value::Type::Unsigned:
            as_unsigned = other.as_unsigned;
            break;
        case rpn_value::Type::Float:
            as_float = other.as_float;
            break;
        case rpn_value::Type::String:
            new (&as_string) String(other.as_string);
            break;
        default:
            break;
    }
    type = other.type;
}

// TODO: global flag RPNLIB_STRING_IMPLEMENTATION, default String
//       implement `template <T> _rpn_value_destroy_string(rpn_value&) { ... }`
//       - ~basic_string() for std::String to test on host
//       - ~String() for Arduino String
rpn_value::rpn_value(rpn_value&& other) :
    type(rpn_value::Type::Null)
{
    assign(other);
    if (other.type == rpn_value::Type::String) {
        other.as_string.~String();
    }
    other.type = rpn_value::Type::Null;
}

rpn_value::rpn_value(bool value) :
    as_boolean(value),
    type(rpn_value::Type::Boolean)
{}

rpn_value::rpn_value(rpn_int_t value) :
    as_integer(value),
    type(rpn_value::Type::Integer)
{}

rpn_value::rpn_value(rpn_uint_t value) :
    as_unsigned(value),
    type(rpn_value::Type::Unsigned)
{}

rpn_value::rpn_value(rpn_float_t value) :
    as_float(value),
    type(rpn_value::Type::Float)
{}

rpn_value::rpn_value(const char* value) :
    type(rpn_value::Type::String)
{
    new (&as_string) String(value);
}

rpn_value::rpn_value(const String& value) :
    type(rpn_value::Type::String)
{
    new (&as_string) String(value);
}

rpn_value::rpn_value(String&& value) :
    type(rpn_value::Type::String)
{
    new (&as_string) String(std::move(value));
}

rpn_value::~rpn_value() {
    if (type == rpn_value::Type::String) {
        as_string.~String();
    }
}

void rpn_value::assign(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::Type::Null:
            if (type == rpn_value::Type::String) {
                as_string.~String();
            }
        case rpn_value::Type::Boolean:
            as_boolean = other.as_boolean;
            break;
        case rpn_value::Type::Integer:
            as_integer = other.as_integer;
            break;
        case rpn_value::Type::Unsigned:
            as_unsigned = other.as_unsigned;
            break;
        case rpn_value::Type::Float:
            as_float = other.as_float;
            break;
        case rpn_value::Type::String:
            if (type == rpn_value::Type::String) {
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
        case rpn_value::Type::Boolean:
            return as_boolean;
        case rpn_value::Type::Integer:
            return as_integer != 0L;
        case rpn_value::Type::Unsigned:
            return as_unsigned != 0UL;
        case rpn_value::Type::Float:
            return as_float != 0.0L;
        case rpn_value::Type::String:
            return as_string.length() > 0;
        case rpn_value::Type::Null:
        default:
            break;
    }

    return false;
}

rpn_value::operator rpn_uint_t() const {
    rpn_uint_t result = 0UL;

    // return Null and set err flag when trying to convert Null
    if (isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::Unsigned:
            result = as_unsigned;
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean ? 1UL : 0UL;
            break;
        case rpn_value::Type::Integer:
            if (as_integer >= 0) {
                result = as_integer;
            }
            break;
        case rpn_value::Type::Float:
            if ((std::numeric_limits<rpn_uint_t>::min() <= as_float)
                && (std::numeric_limits<rpn_uint_t>::max() > as_float)) {
                result = static_cast<rpn_uint_t>(as_float);
            }
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator rpn_float_t() const {
    rpn_float_t result = 0.0L;

    // return Null and set err flag when trying to convert Null
    if (isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::Float:
            result = as_float;
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean ? 1.0L : 0.0L;
            break;
        case rpn_value::Type::Integer:
            result = as_integer;
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned;
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator rpn_int_t() const {
    rpn_int_t result = 0;

    // return Null and set err flag when trying to convert Null
    if (isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::Integer:
            result = as_integer;
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean ? 1.0L : 0.0L;
            break;
        case rpn_value::Type::Unsigned:
            if (static_cast<rpn_uint_t>(std::numeric_limits<rpn_int_t>::max()) < as_unsigned) {
                result = as_unsigned;
            }
            break;
        case rpn_value::Type::Float:
            if ((std::numeric_limits<rpn_int_t>::min() <= as_float)
                && (std::numeric_limits<rpn_int_t>::max() > as_float)) {
                result = lround(as_float);
            }
            break;
        default:
            break;
    }

    return result;
}

rpn_value::operator String() const {
    String result("");

    // return Null and set err flag when trying to convert Null
    if (isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::String:
            return as_string;
        case rpn_value::Type::Integer:
            result = String(as_integer);
            break;
        case rpn_value::Type::Unsigned:
            result = String(as_unsigned);
            break;
        case rpn_value::Type::Float:
            result = String(as_float);
            break;
        default:
            break;
    }

    return result;
}

bool rpn_value::operator <(const rpn_value& other) const {
    bool result = false;

    // return Null and set err flag when trying to do logic with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::Float:
            result = as_float < rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            result = as_integer < rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned < rpn_uint_t(other);
            break;
        case rpn_value::Type::String:
        case rpn_value::Type::Boolean:
        default:
            break;
    }

    return result;
}

bool rpn_value::operator >(const rpn_value& other) const {
    bool result = false;

    // return Null and set err flag when trying to do logic with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::Float:
            result = as_float > rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            result = as_integer > rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned > rpn_uint_t(other);
            break;
        case rpn_value::Type::String:
        case rpn_value::Type::Boolean:
        default:
            break;
    }

    return result;
}

bool rpn_value::operator ==(const rpn_value& other) const {
    bool result = false;

    // return Null and set err flag when trying to do logic with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return result;
    }

    switch (type) {
        case rpn_value::Type::String:
            result = (as_string == other.as_string);
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean == bool(other);
            break;
        case rpn_value::Type::Float:
            result = as_float == rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            result = as_integer == rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned == rpn_uint_t(other);
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

    // return Null and set err flag when trying to do math with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return val;
    }

    switch (type) {
        // concat strings
        case rpn_value::Type::String: {
            if (other.type != rpn_value::Type::String) {
                break;
            }
            val.type = rpn_value::Type::String;

            const auto our_size = as_string.length();
            const auto other_size = other.as_string.length();

            new (&val.as_string) String();
            val.as_string.reserve(our_size + other_size + 1);
            val.as_string += as_string;
            val.as_string += other.as_string;
            break;
        }
        // just do generic math
        case rpn_value::Type::Boolean:
            if (other.type != rpn_value::Type::Boolean) {
                break;
            }
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean + other.as_boolean;
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float + rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer + rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned + rpn_uint_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator -(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return val;
    }

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::Type::Boolean:
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean - bool(other);
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float - rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer - rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned - rpn_uint_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator *(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return val;
    }

    // return Null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // jost do generic math
    switch (type) {
        case rpn_value::Type::Boolean:
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean && bool(other);
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float * rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer * rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned * rpn_uint_t(other);
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator /(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isNull() || other.isNull()) {
        rpn_error = RPN_ERROR_VALUE_IS_NULL;
        return val;
    }

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    if (!_rpn_can_divide_by(other)) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::Type::Boolean:
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean / bool(other);
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float / rpn_float_t(other);
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer / rpn_int_t(other);
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned / rpn_uint_t(other);
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
    if (!_rpn_can_divide_by(other)) {
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::Type::Boolean:
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean % other.as_boolean;
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer - (floor(as_integer / rpn_float_t(other.as_integer)) * other.as_integer);
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned - (floor(as_unsigned / rpn_float_t(other.as_unsigned)) * other.as_unsigned);
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float - (floor(as_float / other.as_float) * other.as_float);
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

bool rpn_value::is(Type value) const {
    return (type == value);
}

bool rpn_value::isNull() const {
    return is(rpn_value::Type::Null);
}

bool rpn_value::isBoolean() const {
    return is(rpn_value::Type::Boolean);
}

bool rpn_value::isString() const {
    return is(rpn_value::Type::String);
}

bool rpn_value::isNumber() const {
    return is(rpn_value::Type::Float) || is(rpn_value::Type::Integer) || is(rpn_value::Type::Unsigned);
}

bool rpn_value::isFloat() const {
    return is(rpn_value::Type::Float);
}

bool rpn_value::isInt() const {
    return is(rpn_value::Type::Integer);
}

bool rpn_value::isUint() const {
    return is(rpn_value::Type::Unsigned);
}

