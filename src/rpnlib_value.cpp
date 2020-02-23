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

rpn_value::rpn_value() :
    type(rpn_value::null)
{}

rpn_value::rpn_value(const rpn_value& other) {
    switch (other.type) {
        case rpn_value::charptr:
            as_charptr = strdup(other.as_charptr);
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
        case rpn_value::charptr:
            as_charptr = other.as_charptr;
            other.as_charptr = nullptr;
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
    other.type = rpn_value::null;
    other.as_i32 = 0;
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

rpn_value::rpn_value(char* value) :
    as_charptr(strdup(value)),
    type(rpn_value::charptr)
{}

rpn_value::~rpn_value() {
    if (type == rpn_value::charptr) {
        free(as_charptr);
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
        case rpn_value::charptr:
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

rpn_value::operator char*() const {
    if (type == rpn_value::charptr) {
        return as_charptr;
    }
    return nullptr;
}

bool rpn_value::operator <(const rpn_value& other) {

    if ((type == rpn_value::charptr) || (other.type == rpn_value::charptr)) {
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

bool rpn_value::operator >(const rpn_value& other) {

    if ((type == rpn_value::charptr) || (other.type == rpn_value::charptr)) {
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

bool rpn_value::operator ==(const rpn_value& other) {

    if ((type == rpn_value::charptr) || (other.type == rpn_value::charptr)) {
        return (strcmp(as_charptr, other.as_charptr) == 0);
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

bool rpn_value::operator !=(const rpn_value& other) {
    return not (*this == other);
}

bool rpn_value::operator >=(const rpn_value& other) {
    return (*this == other) || (*this > other);
}

bool rpn_value::operator <=(const rpn_value& other) {
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
    } else if (type == rpn_value::charptr) {
        val.type = rpn_value::charptr;

        const auto our_size = strlen(as_charptr);
        const auto other_size = strlen(other.as_charptr);

        val.as_charptr = (char*) malloc(our_size + other_size + 1);
        char* ptr = val.as_charptr;
        memcpy(ptr, as_charptr, our_size);
        ptr += our_size;
        memcpy(ptr, other.as_charptr, other_size);
        ptr += other_size;
        *ptr = '\0';
    // do generic math
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_boolean = as_i32 + other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_boolean = as_u32 + other.as_u32;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_boolean = as_f64 + other.as_f64;
    }

    return val;
}

rpn_value rpn_value::operator -(const rpn_value& other) {
    rpn_value val;

    // return null when can't do anything to compute the result
    if ((type != other.type) || (type == rpn_value::charptr) || (other.type == rpn_value::charptr)) {
        return val;
    }

    // do generic math
    if (type == rpn_value::boolean) {
        val.type = rpn_value::boolean;
        val.as_boolean = as_boolean - other.as_boolean;
    } else if (type == rpn_value::i32) {
        val.type = rpn_value::i32;
        val.as_boolean = as_i32 - other.as_i32;
    } else if (type == rpn_value::u32) {
        val.type = rpn_value::u32;
        val.as_boolean = as_u32 - other.as_u32;
    } else if (type == rpn_value::f64) {
        val.type = rpn_value::f64;
        val.as_boolean = as_f64 - other.as_f64;
    }

    return val;
}
