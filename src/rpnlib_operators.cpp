/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

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
#include "rpnlib_operators.h"

extern "C" {
    #include "fs_math.h"
}

#define RPN_CONST_PI    3.141593
#define RPN_CONST_E     2.178282

#include <algorithm>
#include <cmath>
#include <utility>
#include <cstdio>

// ----------------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------------

// TODO: move to core?

static rpn_value& _rpn_stack_peek(rpn_context & ctxt, size_t offset = 1) {
    return *((ctxt.stack.end() - offset)->value.get());
}

static void _rpn_stack_eat(rpn_context & ctxt, size_t size = 1) {
    ctxt.stack.erase(ctxt.stack.end() - size, ctxt.stack.end());
}

// libc cmp interface, will only work with the same types

static int _rpn_stack_compare(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    if (top < prev) {
        return -1;
    } else if (top > prev) {
        return 1;
    } else {
        return 0;
    }
}

static int _rpn_stack_compare_or_eq(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    if (top <= prev) {
        return -1;
    } else if (top >= prev) {
        return 1;
    } else {
        return 0;
    }
}

static int _rpn_stack_compare3(rpn_context & ctxt) {
    auto& upper = _rpn_stack_peek(ctxt, 1);
    auto& lower = _rpn_stack_peek(ctxt, 2);
    auto& value = _rpn_stack_peek(ctxt, 3);
    if (value < lower) {
        return -1;
    } else if (value > upper) {
        return 1;
    } else {
        return 0;
    }
}

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

bool _rpn_pi(rpn_context & ctxt) {
    rpn_stack_push(ctxt, RPN_CONST_PI);
    return true;
}

bool _rpn_e(rpn_context & ctxt) {
    rpn_stack_push(ctxt, RPN_CONST_E);
    return true;
}

// ----------------------------------------------------------------------------
// Math
// ----------------------------------------------------------------------------

bool _rpn_sum(rpn_context & ctxt) {
    const auto result =_rpn_stack_peek(ctxt, 1) + _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_substract(rpn_context & ctxt) {
    const auto result =_rpn_stack_peek(ctxt, 1) - _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_times(rpn_context & ctxt) {
    const auto result =_rpn_stack_peek(ctxt, 1) * _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_divide(rpn_context & ctxt) {
    const auto result =_rpn_stack_peek(ctxt, 1) / _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_mod(rpn_context & ctxt) {
    const auto result =_rpn_stack_peek(ctxt, 1) % _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_abs(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    return top.numeric_abs();
}

// ----------------------------------------------------------------------------
// Logic
// ----------------------------------------------------------------------------

bool _rpn_eq(rpn_context & ctxt) {
    const bool result = _rpn_stack_peek(ctxt, 1) == _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_ne(rpn_context & ctxt) {
    const bool result = _rpn_stack_peek(ctxt, 1) != _rpn_stack_peek(ctxt, 2);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_gt(rpn_context & ctxt) {
    const bool result = _rpn_stack_compare(ctxt) == 1;
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_ge(rpn_context & ctxt) {
    const bool result = _rpn_stack_compare_or_eq(ctxt) == 1;
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_lt(rpn_context & ctxt) {
    const bool result = _rpn_stack_compare(ctxt) == -1;
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_le(rpn_context & ctxt) {
    const bool result = _rpn_stack_compare_or_eq(ctxt) == -1;
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

// ----------------------------------------------------------------------------
// Advanced logic
// ----------------------------------------------------------------------------

bool _rpn_cmp(rpn_context & ctxt) {
    rpn_stack_push(ctxt, _rpn_stack_compare(ctxt));
    return true;
};    

bool _rpn_cmp3(rpn_context & ctxt) {
    rpn_stack_push(ctxt, _rpn_stack_compare3(ctxt));
    return true;
};    

// Allow indexed access for N+1 stack array
// - top of the stack is array size
// - N next elements are members of the array
// - N+1'th stack value is array index to push onto the stack
// We eat all of the stack and push N'th element. When either index or array size is wrong, do nothing
bool _rpn_index(rpn_context & ctxt) {
    const auto stack_size = rpn_stack_size(ctxt);

    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto size = double(top);
    if (stack_size < size + 1) {
        return false;
    }

    const auto& bottom = _rpn_stack_peek(ctxt, size + 2);
    const auto index = double(bottom);
    if ((index + 1) > size) {
        return false;
    }

    const auto pick = _rpn_stack_peek(ctxt, size + 1 - index);
    _rpn_stack_eat(ctxt, size + 2);

    rpn_stack_push(ctxt, pick);

    return false;
}

bool _rpn_map(rpn_context & ctxt) {
    
    auto value = _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 1);

    auto& from_low = _rpn_stack_peek(ctxt, 1);
    auto& from_high = _rpn_stack_peek(ctxt, 2);
    auto& to_low = _rpn_stack_peek(ctxt, 3);
    auto& to_high = _rpn_stack_peek(ctxt, 4);

    if (from_high == from_low) return false;
    if (value < from_low) value = from_low;
    if (value > from_high) value = from_high;
    value = to_low + (value - from_low) * (to_high - to_low) / (from_high - from_low);

    _rpn_stack_eat(ctxt, 4);
    rpn_stack_push(ctxt, value);

    return true;

};

bool _rpn_constrain(rpn_context & ctxt) {

    const auto upper = _rpn_stack_peek(ctxt, 1);
    const auto lower = _rpn_stack_peek(ctxt, 2);
    const auto value = _rpn_stack_peek(ctxt, 3);
    _rpn_stack_eat(ctxt, 3);

    if (value < lower) {
        rpn_stack_push(ctxt, lower);
    } else if (value > upper) {
        rpn_stack_push(ctxt, upper);
    } else {
        rpn_stack_push(ctxt, value);
    }

    return true;
};    

// ----------------------------------------------------------------------------
// Boolean
// ----------------------------------------------------------------------------

bool _rpn_and(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);
    if (!top.is_number() || !prev.is_number()) {
        return false;
    }

    const bool result = (!top.is_number_zero() && !prev.is_number_zero());
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_or(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);
    if (!top.is_number() || !prev.is_number()) {
        return false;
    }

    const bool result = (!top.is_number_zero() || !prev.is_number_zero());
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_xor(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);
    if (!top.is_number() || !prev.is_number()) {
        return false;
    }

    const bool result = (!top.is_number_zero() ^ !prev.is_number_zero());
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_not(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const bool result = top.is_number_zero();
    _rpn_stack_eat(ctxt, 1);
    rpn_stack_push(ctxt, result);
    return true;
}

// ----------------------------------------------------------------------------
// Casting
// ----------------------------------------------------------------------------

bool _rpn_round(rpn_context & ctxt) {
    
    const auto& decimals = _rpn_stack_peek(ctxt, 1);
    const auto& value = _rpn_stack_peek(ctxt, 2);

    if ((decimals.type != rpn_value::f64) || (value.type != rpn_value::f64)) {
        return false;
    }
    
    int multiplier = 1;
    for (int i = 0; i < round(decimals.as_f64); ++i) {
        multiplier *= 10;
    }

    rpn_stack_push(ctxt, double(round(value.as_f64 * multiplier + 0.5L) / multiplier));

    return true;

}

bool _rpn_ceil(rpn_context & ctxt) {
    auto& value = _rpn_stack_peek(ctxt, 1);
    if (value.type != rpn_value::f64) {
        return false;
    }

    value.as_f64 = ceil(value);
    return true;
}

bool _rpn_floor(rpn_context & ctxt) {
    auto& value = _rpn_stack_peek(ctxt, 1);
    if (value.type != rpn_value::f64) {
        return false;
    }

    value.as_f64 = floor(value);
    return true;
}

// ----------------------------------------------------------------------------
// Conditionals
// ----------------------------------------------------------------------------

bool _rpn_ifn(rpn_context & ctxt) {
    const auto c = _rpn_stack_peek(ctxt, 1);
    const auto b = _rpn_stack_peek(ctxt, 2);
    const auto a = _rpn_stack_peek(ctxt, 3);

    _rpn_stack_eat(ctxt, 3);
    rpn_stack_push(ctxt, !a.is_number_zero() ? b : c);

    return true;
}

bool _rpn_end(rpn_context & ctxt) {
    const auto value = _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 1);
    return value.is_number_zero();
}

// ----------------------------------------------------------------------------
// Stack
// ----------------------------------------------------------------------------

// [a] -> [a a]
bool _rpn_dup(rpn_context & ctxt) {
    rpn_stack_push(ctxt, _rpn_stack_peek(ctxt, 1));
    return true;
}

// [a b] -> [a b a b]
bool _rpn_dup2(rpn_context & ctxt) {
    rpn_stack_push(ctxt, _rpn_stack_peek(ctxt, 1));
    rpn_stack_push(ctxt, _rpn_stack_peek(ctxt, 3));
    return true;
}

// [a b] -> [a b a]
bool _rpn_over(rpn_context & ctxt) {
    rpn_stack_push(ctxt, _rpn_stack_peek(ctxt, 2));
    return true;
}

// [a b] -> [b a]
bool _rpn_swap(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    std::swap(top, prev);
    return true;
}

// [a b c] -> [c a b]
bool _rpn_unrot(rpn_context & ctxt) {
    const auto c = _rpn_stack_peek(ctxt, 1);
    const auto b = _rpn_stack_peek(ctxt, 1);
    const auto a = _rpn_stack_peek(ctxt, 1);

    rpn_stack_push(ctxt, c);
    rpn_stack_push(ctxt, a);
    rpn_stack_push(ctxt, b);

    return true;
}

bool _rpn_rot(rpn_context & ctxt) {
    const auto c = _rpn_stack_peek(ctxt, 1);
    const auto b = _rpn_stack_peek(ctxt, 1);
    const auto a = _rpn_stack_peek(ctxt, 1);

    rpn_stack_push(ctxt, b); 
    rpn_stack_push(ctxt, c);
    rpn_stack_push(ctxt, a);

    return true;
}

bool _rpn_drop(rpn_context & ctxt) {
    _rpn_stack_eat(ctxt, 1);
    return true;
}

bool _rpn_depth(rpn_context & ctxt) {
    rpn_stack_push(ctxt, uint32_t(rpn_stack_size(ctxt)));
    return true;
}

bool _rpn_exists(rpn_context & ctxt) {
    return (ctxt.stack.back().value->type != rpn_value::null);
}

bool _rpn_assign(rpn_context & ctxt) {
    auto stack_first = *(ctxt.stack.end() - 1);
    auto stack_second = *(ctxt.stack.end() - 2);
    *stack_first.value.get() = *stack_second.value.get();
    ctxt.stack.erase(ctxt.stack.end() - 2);
    return true;
}

bool _rpn_print(rpn_context & ctxt) {
    auto top = ctxt.stack.back();

    if (!_rpn_debug_callback) {
        return false;
    }

    auto& val = *(top.value.get());

    char buffer[128];
    int offset = 0;

    if (top.variable) {
        offset = snprintf(buffer, 128, "$%s = ", top.variable->name.c_str());
    }

    switch (val.type) {
        case rpn_value::boolean:
            sprintf(buffer + offset, "%s", val.as_boolean ? "true" : "false");
            break;
        case rpn_value::i32:
            sprintf(buffer + offset, "%d", val.as_i32);
            break;
        case rpn_value::u32:
            sprintf(buffer + offset, "%u", val.as_u32);
            break;
        case rpn_value::f64:
            sprintf(buffer + offset, "%f", val.as_f64);
            break;
        case rpn_value::string:
            sprintf(buffer + offset, "\"%s\"", val.as_string.c_str());
            break;
        case rpn_value::null:
        default:
            sprintf(buffer + offset, "null");
            break;
    }

    _rpn_debug_callback(ctxt, buffer);

    return true;
}
