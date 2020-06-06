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

rpn_value_error _rpn_can_divide_by(const rpn_value& value) {
    rpn_value_error result = rpn_value_error::Ok;

    switch (value.type) {
    case rpn_value::Type::Float: {
        auto as_float = value.toFloat();
        if (std::isinf(as_float) || std::isnan(as_float)) {
            return rpn_value_error::IEEE754;
        }
        if (as_float == 0.0) {
            result = rpn_value_error::DivideByZero;
        }
        break;
    }
    case rpn_value::Type::Null:
        return rpn_value_error::IsNull;
    case rpn_value::Type::Integer:
        if (value.toInt() == 0) {
            result = rpn_value_error::DivideByZero;
        }
        break;
    case rpn_value::Type::Unsigned:
        if (value.toUint() == 0UL) {
            result = rpn_value_error::DivideByZero;
        }
        break;
    case rpn_value::Type::Error:
    case rpn_value::Type::Boolean:
    case rpn_value::Type::String:
        result = rpn_value_error::InvalidOperation;
        break;
    }

    return result;
}

}

rpn_value::rpn_value() :
    type(rpn_value::Type::Null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    if (other.type == rpn_value::Type::String) {
        new (&as_string) String(other.as_string);
        type = Type::String;
    } else {
        assignPrimitive(other);
    }
}

// TODO: global flag RPNLIB_STRING_IMPLEMENTATION, default String
//       implement `template <T> _rpn_value_destroy_string(rpn_value&) { ... }`
//       - ~basic_string() for std::String to test on host
//       - ~String() for Arduino String
rpn_value::rpn_value(rpn_value&& other) noexcept {
    if (other.type == rpn_value::Type::String) {
        type = Type::String;
        new (&as_string) String(std::move(other.as_string));
        other.as_string.~String();
    } else {
        assignPrimitive(other);
    }
    other.type = rpn_value::Type::Null;
}

rpn_value::rpn_value(bool value) :
    type(rpn_value::Type::Boolean),
    as_boolean(value)
{}

rpn_value::rpn_value(rpn_value_error value) :
    type(rpn_value::Type::Error),
    as_error(value)
{}

rpn_value::rpn_value(rpn_int_t value) :
    type(rpn_value::Type::Integer),
    as_integer(value)
{}

rpn_value::rpn_value(rpn_uint_t value) :
    type(rpn_value::Type::Unsigned),
    as_unsigned(value)
{}

rpn_value::rpn_value(rpn_float_t value) :
    type(rpn_value::Type::Float),
    as_float(value)
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

void rpn_value::assignPrimitive(const rpn_value& other) noexcept {
    switch (other.type) {
    case rpn_value::Type::Null:
        break;
    case rpn_value::Type::Error:
        as_error = other.as_error;
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
        break;
    }
    type = other.type;
}

void rpn_value::assign(const rpn_value& other) noexcept {
    if (other.type == Type::String) {
        if (type == rpn_value::Type::String) {
            as_string = other.as_string;
        } else {
            new (&as_string) String(other.as_string);
        }
    } else {
        assignPrimitive(other);
    }
    type = other.type;
}

rpn_error rpn_value::toError() const {
    rpn_error result;

    switch (type) {
    case rpn_value::Type::Error:
        result = as_error;
        break;
    case rpn_value::Type::Boolean:
    case rpn_value::Type::Integer:
    case rpn_value::Type::Unsigned:
    case rpn_value::Type::Float:
    case rpn_value::Type::String:
    case rpn_value::Type::Null:
        break;
    }

    return result;
}

bool rpn_value::toBoolean() const {
    switch (type) {
    case rpn_value::Type::Boolean:
        return as_boolean;
    case rpn_value::Type::Integer:
        return as_integer != 0L;
    case rpn_value::Type::Unsigned:
        return as_unsigned != 0UL;
    case rpn_value::Type::Float:
        return as_float != 0.0;
    case rpn_value::Type::String:
        return as_string.length() > 0;
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
        break;
    }

    return false;
}

rpn_uint_t rpn_value::toUint() const {
    rpn_uint_t result = 0UL;

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

rpn_float_t rpn_value::toFloat() const {
    rpn_float_t result = 0.0;

    switch (type) {
    case rpn_value::Type::Float:
        result = as_float;
        break;
    case rpn_value::Type::Boolean:
        result = as_boolean ? 1.0 : 0.0;
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

rpn_int_t rpn_value::toInt() const {
    rpn_int_t result = 0;

    switch (type) {
        case rpn_value::Type::Integer:
            result = as_integer;
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean ? 1.0 : 0.0;
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

String rpn_value::toString() const {
    String result("");

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

rpn_value::operator bool() const {
    return (!isNull() && !isError());
}

bool rpn_value::operator <(const rpn_value& other) const {
    bool result = false;

    // return Null when trying to do logic with Null
    if (isNull() || other.isNull()) {
        return result;
    }

    switch (type) {
        case rpn_value::Type::Float:
            result = as_float < other.toFloat();
            break;
        case rpn_value::Type::Integer:
            result = as_integer < other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned < other.toUint();
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

    // return Null when trying to do logic with Null
    if (isNull() || other.isNull()) {
        return result;
    }

    switch (type) {
        case rpn_value::Type::Float:
            result = as_float > other.toFloat();
            break;
        case rpn_value::Type::Integer:
            result = as_integer > other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned > other.toUint();
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

    // return Null when trying to do logic with Null
    if (isNull() || other.isNull()) {
        return result;
    }

    switch (type) {
        case rpn_value::Type::String:
            result = (as_string == other.as_string);
            break;
        case rpn_value::Type::Boolean:
            result = as_boolean == other.toBoolean();
            break;
        case rpn_value::Type::Float:
            result = as_float == other.toFloat();
            break;
        case rpn_value::Type::Integer:
            result = as_integer == other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            result = as_unsigned == other.toUint();
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
    if (isError()) {
        val = *this;
        return val;
    }

    if (isNull() || other.isNull()) {
        val = rpn_value(rpn_value_error::InvalidOperation);
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
            val.as_float = as_float + other.toFloat();
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer + other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned + other.toUint();
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator -(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isError()) {
        val = *this;
        return val;
    }

    if (isNull() || other.isNull()) {
        val = rpn_value(rpn_value_error::IsNull);
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
            val.as_boolean = as_boolean - other.toBoolean();
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float - other.toFloat();
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer - other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned - other.toUint();
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator *(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isError()) {
        val = *this;
        return val;
    }

    if (isNull() || other.isNull()) {
        val = rpn_value(rpn_value_error::IsNull);
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
            val.as_boolean = as_boolean && other.toBoolean();
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float * other.toFloat();
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer * other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned * other.toUint();
            break;
        default:
            break;
    }

    return val;
}

rpn_value rpn_value::operator /(const rpn_value& other) {
    rpn_value val;

    // return Null and set err flag when trying to do math with Null
    if (isError()) {
        val = *this;
        return val;
    }

    if (isNull() || other.isNull()) {
        val = rpn_value(rpn_value_error::IsNull);
        return val;
    }

    // return null when can't do anything to compute the result
    if (!isNumber() && !isBoolean()) {
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    auto err = _rpn_can_divide_by(other);
    if (err != rpn_value_error::Ok) {
        val = rpn_value(err);
        return val;
    }

    // just do generic math
    switch (type) {
        case rpn_value::Type::Boolean:
            val.type = rpn_value::Type::Boolean;
            val.as_boolean = as_boolean / other.toBoolean();
            break;
        case rpn_value::Type::Float:
            val.type = rpn_value::Type::Float;
            val.as_float = as_float / other.toFloat();
            break;
        case rpn_value::Type::Integer:
            val.type = rpn_value::Type::Integer;
            val.as_integer = as_integer / other.toInt();
            break;
        case rpn_value::Type::Unsigned:
            val.type = rpn_value::Type::Unsigned;
            val.as_unsigned = as_unsigned / other.toUint();
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
    auto err = _rpn_can_divide_by(other);
    if (err != rpn_value_error::Ok) {
        val = rpn_value(err);
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

bool rpn_value::isError() const {
    return is(rpn_value::Type::Error);
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

