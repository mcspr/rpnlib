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

#include <string>
#include <cstring>
#include <cmath>
#include <cstdlib>

// TODO: implement fs_math operations

namespace {

rpn_value_error _rpn_can_divide_by(const rpn_value& value) {
    rpn_value_error result = rpn_value_error::Ok;

    switch (value.type) {

    case rpn_value::Type::Float: {
        auto conversion = value.checkedToFloat();
        if (!conversion.ok()) {
            result = conversion.error();
            break;
        }

        rpn_float as_float = conversion.value();
        if (std::isinf(as_float) || std::isnan(as_float)) {
            result = rpn_value_error::IEEE754;
            break;
        }

        if (static_cast<rpn_float>(0.0) == as_float) {
            result = rpn_value_error::DivideByZero;
        }

        break;
    }

    case rpn_value::Type::Null:
        return rpn_value_error::IsNull;

    case rpn_value::Type::Integer: {
        auto conversion = value.checkedToInt();
        if (!conversion.ok()) {
            result = conversion.error();
            break;
        }

        if (static_cast<rpn_int>(0) == conversion.value()) {
            result = rpn_value_error::DivideByZero;
            break;
        }

        break;
    }

    case rpn_value::Type::Unsigned: {
        auto conversion = value.checkedToInt();
        if (!conversion.ok()) {
            result = conversion.error();
            break;
        }

        if (static_cast<rpn_uint>(0ul) == conversion.value()) {
            result = rpn_value_error::DivideByZero;
            break;
        }
        break;
    }

    case rpn_value::Type::Error:
    case rpn_value::Type::Boolean:
    case rpn_value::Type::String:
        result = rpn_value_error::InvalidOperation;
        break;

    }

    return result;
}

rpn_value_error _rpn_can_call_operator(const rpn_value& lhs, const rpn_value& rhs) {
    if (lhs.isError()) {
        return lhs.toError();
    }

    if (rhs.isError()) {
        return rhs.toError();
    }

    if (lhs.isNull() || rhs.isNull()) {
        return rpn_value_error::IsNull;
    }

    return rpn_value_error::Ok;
}

rpn_value_error _rpn_can_call_math_operator(const rpn_value& lhs, const rpn_value& rhs) {
    auto err = _rpn_can_call_operator(lhs, rhs);
    if (err != rpn_value_error::Ok) {
        return err;
    }

    if (lhs.isNumber() && (rhs.isNumber() || rhs.isBoolean())) {
        return rpn_value_error::Ok;
    }

    if (lhs.isBoolean() && (rhs.isNumber() || rhs.isBoolean())) {
        return rpn_value_error::Ok;
    }

    return rpn_value_error::InvalidOperation;
}

} // namespace

rpn_value::rpn_value() :
    type(rpn_value::Type::Null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    if (other.type == rpn_value::Type::String) {
        new (&as_string) std::string(other.as_string);
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
        new (&as_string) std::string(std::move(other.as_string));
        other.as_string.~basic_string();
    } else {
        assignPrimitive(other);
    }
    other.type = rpn_value::Type::Null;
}

rpn_value::rpn_value(bool value) :
    type(rpn_value::Type::Boolean),
    as_boolean(value)
{}

rpn_value::rpn_value(rpn_value_error error) :
    type(rpn_value::Type::Error),
    as_error(error)
{}

rpn_value::rpn_value(rpn_int value) :
    type(rpn_value::Type::Integer),
    as_integer(value)
{}

rpn_value::rpn_value(rpn_uint value) :
    type(rpn_value::Type::Unsigned),
    as_unsigned(value)
{}

rpn_value::rpn_value(rpn_float value) :
    type(rpn_value::Type::Float),
    as_float(value)
{}

rpn_value::rpn_value(const char* value) :
    type(rpn_value::Type::String)
{
    new (&as_string) std::string(value);
}

rpn_value::rpn_value(const std::string& value) :
    type(rpn_value::Type::String)
{
    new (&as_string) std::string(value);
}

rpn_value::rpn_value(std::string&& value) :
    type(rpn_value::Type::String)
{
    new (&as_string) std::string(std::move(value));
}

rpn_value::~rpn_value() {
    if (type == rpn_value::Type::String) {
        as_string.~basic_string();
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
            new (&as_string) std::string(other.as_string);
        }
    } else {
        assignPrimitive(other);
    }
    type = other.type;
}

rpn_value_error rpn_value::toError() const {
    auto result = rpn_value_error::NotAnError;

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
    bool result { false };

    switch (type) {
    case rpn_value::Type::Boolean:
        result = as_boolean;
        break;
    case rpn_value::Type::Integer:
        result = static_cast<rpn_int>(0) != as_integer;
        break;
    case rpn_value::Type::Unsigned:
        result = static_cast<rpn_uint>(0ul) != as_unsigned;
        break;
    case rpn_value::Type::Float:
        result = static_cast<rpn_float>(0.0) != as_float;
        break;
    case rpn_value::Type::String: {
        using size_type = decltype(std::declval<std::string>().length());
        result = static_cast<size_type>(0ul) < as_string.length();
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
        break;
    }

    return result;
}

rpn_optional<rpn_int> rpn_value::checkedToInt() const {
    rpn_optional<rpn_int> result(
        static_cast<rpn_int>(0),
        rpn_value_error::ImpossibleConversion
    );

    switch (type) {
    case rpn_value::Type::Integer:
        result = as_integer;
        break;
    case rpn_value::Type::Boolean:
        result = as_boolean
            ? static_cast<rpn_int>(1)
            : static_cast<rpn_int>(0);
        break;
    case rpn_value::Type::Unsigned:
        if (std::numeric_limits<rpn_int>::digits <= std::numeric_limits<rpn_uint>::digits) {
            constexpr rpn_uint mask = std::numeric_limits<rpn_int>::max();
            if ((as_unsigned & mask) != as_unsigned) {
                break;
            }
            result = static_cast<rpn_int>(as_unsigned);
            break;
        }
        result = rpn_value_error::OutOfRangeConversion;
        break;
    case rpn_value::Type::Float: {
        constexpr rpn_float upper = std::numeric_limits<rpn_int>::max();
        constexpr rpn_float lower = std::numeric_limits<rpn_int>::min();
        if ((lower <= as_float) && (as_float <= upper)) {
            result = std::round(as_float);
            break;
        }
        result = rpn_value_error::OutOfRangeConversion;
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return result;
}

rpn_int rpn_value::toInt() const {
    return isInt() ? as_integer : checkedToInt().value();
}

rpn_optional<rpn_uint> rpn_value::checkedToUint() const {
    rpn_optional<rpn_uint> result(
        static_cast<rpn_uint>(0ul),
        rpn_value_error::ImpossibleConversion
    );

    switch (type) {
    case rpn_value::Type::Unsigned:
        result = as_unsigned;
        break;
    case rpn_value::Type::Boolean:
        result = as_boolean
            ? static_cast<rpn_uint>(1UL)
            : static_cast<rpn_uint>(0UL);
        break;
    case rpn_value::Type::Integer:
        if (static_cast<rpn_int>(0) <= as_integer) {
            if (std::numeric_limits<rpn_int>::digits > std::numeric_limits<rpn_uint>::digits) {
                constexpr auto mask = std::numeric_limits<rpn_int>::max();
                if ((as_integer & mask) != as_integer) {
                    break;
                }
            }
            result = static_cast<rpn_uint>(as_integer);
            break;
        }
        result = rpn_value_error::OutOfRangeConversion;
        break;
    case rpn_value::Type::Float: {
        constexpr rpn_float upper = std::numeric_limits<rpn_uint>::max();
        constexpr rpn_float lower = std::numeric_limits<rpn_uint>::min();
        if ((as_float >= lower) && (as_float <= upper)) {
            result = std::round(as_float);
            break;
        }
        result = rpn_value_error::OutOfRangeConversion;
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return result;
}

rpn_uint rpn_value::toUint() const {
    return isUint() ? as_unsigned : checkedToUint().value();
}

rpn_optional<rpn_float> rpn_value::checkedToFloat() const {
    rpn_optional<rpn_float> result(
        static_cast<rpn_float>(0.0),
        rpn_value_error::ImpossibleConversion
    );

    switch (type) {
    case rpn_value::Type::Float:
        result = as_float;
        break;
    case rpn_value::Type::Boolean:
        result = as_boolean
            ? static_cast<rpn_float>(1.0)
            : static_cast<rpn_float>(0.0);
        break;
    case rpn_value::Type::Integer:
        result = static_cast<rpn_float>(as_integer);
        break;
    case rpn_value::Type::Unsigned:
        result = static_cast<rpn_float>(as_unsigned);
        break;
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return result;
}

rpn_float rpn_value::toFloat() const {
    return isFloat() ? as_float : checkedToFloat().value();
}

std::string rpn_value::toString() const {
    std::string result;

    switch (type) {
    case rpn_value::Type::Null:
        result = "null";
        break;
    case rpn_value::Type::Error: {
        char buffer[10 + (4 * sizeof(int))];
        sprintf(buffer, "error %d", static_cast<int>(as_error));
        result = buffer;
        break;
    }
    case rpn_value::Type::Boolean:
        result = as_boolean ? "true" : "false";
        break;
    case rpn_value::Type::Integer: {
        char buffer[4 * sizeof(rpn_int)];
        snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(as_integer));
        result = buffer;
        break;
    }
    case rpn_value::Type::Unsigned: {
        char buffer[4 * sizeof(rpn_uint)];
        snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(as_unsigned));
        result = buffer;
        break;
    }
    case rpn_value::Type::Float: {
        char buffer[20 + std::numeric_limits<rpn_float>::max_exponent10];
        snprintf(buffer, sizeof(buffer), "%f", as_float);
        result = buffer;
        break;
    }
    case rpn_value::Type::String:
        result = as_string;
        break;
    }

    return result;
}

rpn_value::operator bool() const {
    return (!isNull() && !isError());
}

bool rpn_value::operator<(const rpn_value& other) const {
    bool result = false;

    // return Null when trying to do logic with Null
    if (isNull() || other.isNull()) {
        return result;
    }

    switch (type) {
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (conversion.ok()) {
            result = as_integer < conversion.value();
        }
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (conversion.ok()) {
            result = as_unsigned < conversion.value();
        }
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (conversion.ok()) {
            result = as_float < conversion.value();
        }
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::Boolean:
    case rpn_value::Type::String:
        break;
    }

    return result;
}

bool rpn_value::operator>(const rpn_value& other) const {
    bool result = false;

    // return Null when trying to do logic with Null
    if (isNull() || other.isNull()) {
        return result;
    }

    switch (type) {
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (conversion.ok()) {
            result = as_integer > conversion.value();
        }
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (conversion.ok()) {
            result = as_unsigned > conversion.value();
        }
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (conversion.ok()) {
            result = as_float > conversion.value();
        }
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::Boolean:
    case rpn_value::Type::String:
        break;
    }

    return result;
}

bool rpn_value::operator==(const rpn_value& other) const {
    bool result = false;

    switch (type) {
    case rpn_value::Type::Null:
        result = other.isNull();
        break;
    case rpn_value::Type::Error:
        result = as_error == other.toError();
        break;
    case rpn_value::Type::Boolean:
        result = as_boolean == other.toBoolean();
        break;
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (conversion.ok()) {
            result = as_integer == conversion.value();
        }
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (conversion.ok()) {
            result = as_unsigned == conversion.value();
        }
        break;
    }
    case rpn_value::Type::Float: {
        if (std::isinf(as_float) || std::isnan(as_float)) {
            break;
        }

        if (other.isFloat() && (std::isinf(other.as_float) || std::isnan(other.as_float))) {
            break;
        }

        // TODO: slightly reduce epsilon value to slightly (lol) widen the margin for error
        // ref:
        // - https://randomascii.wordpress.com/category/floating-point/
        // - https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
        // - https://randomascii.wordpress.com/2012/06/26/doubles-are-not-floats-so-dont-compare-them/
        // - https://www.embeddeduse.com/2019/08/26/qt-compare-two-floats/
        constexpr auto epsilon = std::numeric_limits<rpn_float>::epsilon();
        if (other.isFloat()) {
            result = std::abs(as_float - other.as_float) <= epsilon;
            break;
        }

        auto conversion = other.checkedToFloat();
        if (conversion.ok()) {
            result = std::abs(as_float - conversion.value()) <= epsilon;
            break;
        }

        break;
    }
    case rpn_value::Type::String:
        result = (as_string == other.as_string);
        break;
    }

    return result;
}

bool rpn_value::operator!=(const rpn_value& other) const {
    return not (*this == other);
}

bool rpn_value::operator>=(const rpn_value& other) const {
    return (*this == other) || (*this > other);
}

bool rpn_value::operator<=(const rpn_value& other) const {
    return (*this == other) || (*this < other);
}

rpn_value rpn_value::operator+(const rpn_value& other) {
    rpn_value val;

    // **Notice!**
    // strings are valid here, so we don't have explicit check for number / bool
    auto error = _rpn_can_call_operator(*this, other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
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

        new (&val.as_string) std::string();
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
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Integer;
        val.as_integer = as_integer + conversion.value();
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Unsigned;
        val.as_unsigned = as_unsigned + conversion.value();
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Float;
        val.as_float = as_float + conversion.value();
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
        break;
    }

    return val;
}

rpn_value rpn_value::operator-(const rpn_value& other) {
    rpn_value val;

    // return Error when operation does not make sense
    auto error = _rpn_can_call_math_operator(*this, other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    // just do generic math
    switch (type) {
    case rpn_value::Type::Boolean:
        val.type = rpn_value::Type::Boolean;
        val.as_boolean = as_boolean - other.toBoolean();
        break;
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Integer;
        val.as_integer = as_integer - conversion.value();
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Unsigned;
        val.as_unsigned = as_unsigned - conversion.value();
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Float;
        val.as_float = as_float - conversion.value();
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return val;
}

rpn_value rpn_value::operator*(const rpn_value& other) {
    rpn_value val;

    // return Error when operation does not make sense
    auto error = _rpn_can_call_math_operator(*this, other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    switch (type) {
    case rpn_value::Type::Boolean:
        val.type = rpn_value::Type::Boolean;
        val.as_boolean = as_boolean && other.toBoolean();
        break;
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Integer;
        val.as_integer = as_integer * conversion.value();
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Unsigned;
        val.as_unsigned = as_unsigned * conversion.value();
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Float;
        val.as_float = as_float * conversion.value();
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return val;
}

rpn_value rpn_value::operator/(const rpn_value& other) {
    rpn_value val;

    // return Error when operation does not make sense
    auto error = _rpn_can_call_math_operator(*this, other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    error = _rpn_can_divide_by(other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    // just do generic math
    switch (type) {
    case rpn_value::Type::Boolean:
        val.type = rpn_value::Type::Boolean;
        val.as_boolean = as_boolean / other.toBoolean();
        break;
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Integer;
        val.as_integer = as_integer / conversion.value();
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Unsigned;
        val.as_unsigned = as_unsigned / conversion.value();
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Float;
        val.as_float = as_float / conversion.value();
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return val;
}

rpn_value rpn_value::operator%(const rpn_value& other) {
    rpn_value val;

    // return Error when operation does not make sense
    auto error = _rpn_can_call_math_operator(*this, other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    // avoid division by zero (previously, RPN_ERROR_DIVIDE_BY_ZERO)
    // technically, we will get either inf or nan with floating point math, but we need operator to do the conversion for this `val` to make sense to the user
    error = _rpn_can_divide_by(other);
    if (rpn_value_error::Ok != error) {
        val = rpn_value(error);
        return val;
    }

    // just do generic math
    switch (type) {
    case rpn_value::Type::Boolean:
        val.type = rpn_value::Type::Boolean;
        val.as_boolean = as_boolean % other.as_boolean;
        break;
    case rpn_value::Type::Integer: {
        auto conversion = other.checkedToInt();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Integer;
        val.as_integer = as_integer - (std::floor(as_integer / static_cast<rpn_float>(conversion.value())) * conversion.value());
        break;
    }
    case rpn_value::Type::Unsigned: {
        auto conversion = other.checkedToUint();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Unsigned;
        val.as_unsigned = as_unsigned - (std::floor(as_unsigned / static_cast<rpn_float>(conversion.value())) * conversion.value());
        break;
    }
    case rpn_value::Type::Float: {
        auto conversion = other.checkedToFloat();
        if (!conversion.ok()) {
            val = rpn_value(conversion.error());
            break;
        }
        val.type = rpn_value::Type::Float;
        val.as_float = as_float - (std::floor(as_float / conversion.value()) * conversion.value());
        break;
    }
    case rpn_value::Type::Null:
    case rpn_value::Type::Error:
    case rpn_value::Type::String:
        break;
    }

    return val;
}

rpn_value& rpn_value::operator=(const rpn_value& other) {
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

