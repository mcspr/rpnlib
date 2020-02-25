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
#include <cstdio>

// TODO: implement fs_math operations

rpn_value::rpn_value() :
    type(rpn_value::null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::string:
            new (&as_string) std::string(other.as_string);
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
    }
    type = other.type;
}

rpn_value::rpn_value(rpn_value&& other) {
    switch (other.type) {
        case rpn_value::string: {
            as_string = std::move(other.as_string);
            other.as_string.~basic_string();
            break;
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
    }
    type = other.type;
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
    printf("%s new() start\n", __PRETTY_FUNCTION__);
    new (&as_string) std::string(value);
    printf("%s new() done -> %s\n", __PRETTY_FUNCTION__, as_string.c_str());
}

rpn_value::rpn_value(const std::string& value) :
    type(rpn_value::string)
{
    printf("%s new()\n", __PRETTY_FUNCTION__);
    new (&as_string) std::string(value);
}

rpn_value::rpn_value(std::string&& value) :
    type(rpn_value::string)
{
    printf("%s new()\n", __PRETTY_FUNCTION__);
    new (&as_string) std::string(std::move(value));
}

rpn_value::~rpn_value() {
    if (type == rpn_value::string) {
        as_string.~basic_string();
    }
}

rpn_value::operator bool() const {
    switch (type) {
        case rpn_value::boolean:
            return as_boolean;
        case rpn_value::i32:
            return as_i32 > 0;
        case rpn_value::u32:
            return as_u32 > 0;
        case rpn_value::f64:
            return as_f64 > 0.0L;
        case rpn_value::string:
        default:
            return true;
    }
}

rpn_value::operator int32_t() const {
    if (type == rpn_value::i32) {
        return as_i32;
    }
    return 0;
}

rpn_value::operator uint32_t() const {
    if (type == rpn_value::u32) {
        return as_u32;
    }
    return 0;
}

rpn_value::operator double() const {
    if (type == rpn_value::f64) {
        return as_f64;
    }
    return 0.0L;
}

rpn_value::operator const char*() const {
    if (type == rpn_value::string) {
        return as_string.c_str();
    }
    return nullptr;
}

bool rpn_value::operator <(const rpn_value& other) const {

    if ((type == rpn_value::string) || (other.type == rpn_value::string)) {
        return false;
    }

    if ((type == rpn_value::boolean) || (other.type == rpn_value::boolean)) {
        return false;
    }

    if (type != other.type) {
        return false;
    }

    switch (type) {
        case rpn_value::i32:
            return (as_i32 < other.as_i32);
        case rpn_value::u32:
            return (as_u32 < other.as_u32);
        case rpn_value::f64:
            return (as_f64 < other.as_f64);
        default:
            return false;
    }
}

bool rpn_value::operator >(const rpn_value& other) const {

    if ((type == rpn_value::string) || (other.type == rpn_value::string)) {
        return false;
    }

    if ((type == rpn_value::boolean) || (other.type == rpn_value::boolean)) {
        return false;
    }

    if (type != other.type) {
        return false;
    }

    switch (type) {
        case rpn_value::i32:
            return (as_i32 > other.as_i32);
        case rpn_value::u32:
            return (as_u32 > other.as_u32);
        case rpn_value::f64:
            return (as_f64 > other.as_f64);
        default:
            return false;
    }
}

bool rpn_value::operator ==(const rpn_value& other) const {

    if ((type == rpn_value::string) || (other.type == rpn_value::string)) {
        return (as_string == other.as_string);
    }

    if (type != other.type) {
        return false;
    }

    switch (type) {
        case rpn_value::boolean:
            return (as_boolean == other.as_boolean);
        case rpn_value::i32:
            return (as_i32 == other.as_i32);
        case rpn_value::u32:
            return (as_u32 == other.as_u32);
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

        new (&val.as_string) std::string();
        val.as_string.reserve(our_size + other_size + 1);
        val.as_string += as_string;
        val.as_string += other.as_string;
    // do generic math
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_i32 = as_i32 + other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_u32 = as_u32 + other.as_u32;
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
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_i32 = as_i32 - other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_u32 = as_u32 - other.as_u32;
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
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_i32 = as_i32 * other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_u32 = as_u32 * other.as_u32;
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
    if (
        ((type == rpn_value::i32) && (other.as_i32 == 0))
        || ((type == rpn_value::u32) && (other.as_u32 == 0))
        || ((type == rpn_value::f64) && (other.as_f64 == 0))
    ) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean / other.as_boolean;
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_i32 = as_i32 / other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_u32 = as_u32 / other.as_u32;
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
    if (
        ((type == rpn_value::i32) && (other.as_i32 == 0))
        || ((type == rpn_value::u32) && (other.as_u32 == 0))
        || ((type == rpn_value::f64) && (other.as_f64 == 0))
    ) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean % other.as_boolean;
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_i32 = as_i32 % other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_u32 = as_u32 % other.as_u32;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_f64 = as_f64 - (as_f64 / other.as_f64) * other.as_f64;
    }

    return val;
}

bool rpn_value::numeric_abs() {
    bool result = false;
    if (type == rpn_value::i32) {
        as_i32 = as_i32 * -1;
        result = true;
    } else if (type == rpn_value::f64) {
        as_f64 = as_f64 * -1.0L;
        result = true;
    }
    return result;
}

bool rpn_value::is_number() const {
    switch (type) {
        case rpn_value::i32:
        case rpn_value::u32:
        case rpn_value::f64:
            return true;
        default:
            return false;
    }
}

bool rpn_value::is_number_zero() const {
    switch (type) {
        case rpn_value::u32:
            return true;
        case rpn_value::i32:
            return (as_i32 == 0);
        case rpn_value::f64:
            return (as_f64 == 0.0L);
        default:
            return false;
    }
}

rpn_value& rpn_value::operator =(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::string:
            if (type == rpn_value::string) {
                as_string = other.as_string;
            } else {
                new (&as_string) std::string(other.as_string);
            }
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
    }
    type = other.type;
    return *this;
}
