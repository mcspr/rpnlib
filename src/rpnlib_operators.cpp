/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 Maxim Prokhorov <prokhorov dot max at outlook dot com>

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
#include "rpnlib_stack.h"
#include "rpnlib_operators.h"

extern "C" {
    #include "fs_math.h"
}

#include <algorithm>
#include <cmath>
#include <utility>
#include <cstdio>
#include <utility>

// These are from <cmath>
#ifdef M_PI
#define RPN_CONST_PI    M_PI
#endif

#ifdef M_E
#define RPN_CONST_E     M_E
#endif

// These come from <Arduino.h> (possibly re-mapped from <cmath>, prefer latter)
#ifndef RPN_CONST_PI
#ifdef PI
#define RPN_CONST_PI    PI
#endif
#endif // RPN_CONST_PI

#ifndef RPN_CONST_E
#ifdef EULER
#define RPN_CONST_E     EULER
#endif
#endif // RPN_CONST_E

// If for whatever reason both fail, fallback to these (float)
#ifndef RPN_CONST_PI
#define RPN_CONST_PI    3.141593
#endif

#ifndef RPN_CONST_E
#define RPN_CONST_E     2.718282
#endif

// anonymous namespace binds all of the functions below to this compilation unit
// this has the same effect as if these functions were `static`
namespace {

// ----------------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------------

// TODO: move to core API?

rpn_value& _rpn_stack_peek(rpn_context & ctxt, size_t offset = 1) {
    return *((ctxt.stack.end() - offset)->value.get());
}

void _rpn_stack_eat(rpn_context & ctxt, size_t size = 1) {
    ctxt.stack.erase(ctxt.stack.end() - size, ctxt.stack.end());
}

// libc cmp interface, depends on implementation of:
// rpn_value::operator <()
// rpn_value::operator >()
// rpn_value::operator >=()
// rpn_value::operator <=()

rpn_int_t _rpn_stack_compare(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    if (prev < top) {
        return -1.0;
    } else if (prev > top) {
        return 1.0;
    } else {
        return 0.0;
    }
}

rpn_int_t _rpn_stack_compare_or_eq(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    if (prev <= top) {
        return -1.0;
    } else if (prev >= top) {
        return 1.0;
    } else {
        return 0.0;
    }
}

rpn_int_t _rpn_stack_compare3(rpn_context & ctxt) {
    auto& upper = _rpn_stack_peek(ctxt, 1);
    auto& lower = _rpn_stack_peek(ctxt, 2);
    auto& value = _rpn_stack_peek(ctxt, 3);
    if (value < lower) {
        return -1.0;
    } else if (value > upper) {
        return 1.0;
    } else {
        return 0.0;
    }
}

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// Pushes specified predefined const values onto the stack
// TODO: should these be `$pi` and `$e` instead?

bool _rpn_pi(rpn_context & ctxt) {
    static auto value = rpn_value { RPN_CONST_PI };
    rpn_stack_push(ctxt, value);
    return true;
}

bool _rpn_e(rpn_context & ctxt) {
    static auto value = rpn_value { RPN_CONST_E };
    rpn_stack_push(ctxt, value);
    return true;
}

// ----------------------------------------------------------------------------
// Math
// ----------------------------------------------------------------------------

// Operators accept [a b] and do `a` OPERATION `b`
// Pushes `f64` value as the result
// Eats both stack values

bool _rpn_sum(rpn_context & ctxt) {
    const auto result = _rpn_stack_peek(ctxt, 2) + _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_substract(rpn_context & ctxt) {
    const auto result = _rpn_stack_peek(ctxt, 2) - _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_times(rpn_context & ctxt) {
    const auto result = _rpn_stack_peek(ctxt, 2) * _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_divide(rpn_context & ctxt) {
    const auto result = _rpn_stack_peek(ctxt, 2) / _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_mod(rpn_context & ctxt) {
    const auto result = _rpn_stack_peek(ctxt, 2) % _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_abs(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    if (!top.isNumber()) {
        return false;
    }
    rpn_value result { rpn_float_t(top) * -1.0 };
    _rpn_stack_eat(ctxt, 1);
    rpn_stack_push(ctxt, std::move(result));
    return true;
}

// ----------------------------------------------------------------------------
// Logic
// ----------------------------------------------------------------------------

// Operators accept [a b] and do `a` OPERATION `b`
// Pushes `boolean` value of the comparison
// Eats both stack values

bool _rpn_eq(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_peek(ctxt, 2) == _rpn_stack_peek(ctxt, 1) };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_ne(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_peek(ctxt, 2) != _rpn_stack_peek(ctxt, 1) };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_gt(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare(ctxt) == 1 };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_ge(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare_or_eq(ctxt) == 1 };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_lt(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare(ctxt) == -1 };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

bool _rpn_le(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare_or_eq(ctxt) == -1 };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
}

// ----------------------------------------------------------------------------
// Advanced logic
// ----------------------------------------------------------------------------

// [a b] -> [c]
// Where `c` is:
// - -1 if `a` < `b`
// - 1 if `a` > `b`
// - 0 if `a` == `b`
bool _rpn_cmp(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare(ctxt) };
    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, result);
    return true;
};    

// [a b c] -> [d]
// Where `d` is
// - -1 if `a` < `b`
// - 1 if `a` > `c`
// - 0 if none of the above match
bool _rpn_cmp3(rpn_context & ctxt) {
    rpn_value result { _rpn_stack_compare3(ctxt) };
    _rpn_stack_eat(ctxt, 3);
    rpn_stack_push(ctxt, result);
    return true;
};    

// [a ... x] -> [y]
// Allow indexed access for N+1 stack array, where:
// - `x` is the array size
// - ... next elements are members of the array
// - `a` is the array[index] to push onto the stack
// We eat all of the stack and push N'th element. When either index or array size is wrong, do nothing
bool _rpn_index(rpn_context & ctxt) {
    const auto stack_size = rpn_stack_size(ctxt);

    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto size = rpn_float_t(top);
    if (size < 0) {
        return false;
    }
    if (stack_size < size + 1) {
        return false;
    }

    const auto& bottom = _rpn_stack_peek(ctxt, size + 2);
    const auto index = rpn_float_t(bottom);
    if ((index + 1) > size) {
        return false;
    }

    const auto pick = _rpn_stack_peek(ctxt, size + 1 - index);
    _rpn_stack_eat(ctxt, size + 2);

    rpn_stack_push(ctxt, pick);

    return true;
}

// [a b c d e] -> [x]
// maps value `a` from range `b`:`c` to `d`:`e`
// stops execution when `c` and `d` are equal
bool _rpn_map(rpn_context & ctxt) {
    auto value = _rpn_stack_peek(ctxt, 5);
    auto from_low = _rpn_stack_peek(ctxt, 4);
    auto from_high = _rpn_stack_peek(ctxt, 3);
    auto to_low = _rpn_stack_peek(ctxt, 2);
    auto to_high = _rpn_stack_peek(ctxt, 1);

    if (from_high == from_low) return false;
    if (value < from_low) value = from_low;
    if (value > from_high) value = from_high;
    value = to_low + (value - from_low) * (to_high - to_low) / (from_high - from_low);

    _rpn_stack_eat(ctxt, 5);
    rpn_stack_push(ctxt, value);

    return true;
};

// [a b c] -> [d]
// constrains `a` between values `b` (lower bound) and `c` (upper bound)
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

// This implements general boolean logic on top of basic types.
// Fundamental numeric type is floating point, so we can't utilize bitwise
// logical operators like `|`, `&` directly on the number.
// Instead, we cast the number to `bool` first.

// [a b] -> [c]
// pushes boolean value of `b` && `a`
bool _rpn_and(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);

    rpn_value result {bool(top) && bool(prev)};
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

// [a b] -> [c]
// pushes boolean value of `b` || `a`
bool _rpn_or(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);

    rpn_value result {bool(top) || bool(prev)};
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

// [a b] -> [c]
// pushes boolean value of `b` ^ `a`
bool _rpn_xor(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    const auto& prev = _rpn_stack_peek(ctxt, 2);

    rpn_value result { bool(bool(top) ^ bool(prev)) };
    _rpn_stack_eat(ctxt, 2);

    rpn_stack_push(ctxt, result);
    return true;
}

// [a] -> [b]
// pushes inverse boolean value of `a`
bool _rpn_not(rpn_context & ctxt) {
    const auto& top = _rpn_stack_peek(ctxt, 1);
    rpn_value result { !bool(top) };
    _rpn_stack_eat(ctxt, 1);
    rpn_stack_push(ctxt, result);
    return true;
}

// ----------------------------------------------------------------------------
// Casting
// ----------------------------------------------------------------------------

// [a b] -> [c], where a is equal to round(a) and `b` is number of decimal places
// stops execution when either `a` or `b` are not numbers
bool _rpn_round(rpn_context & ctxt) {
    
    const auto& decimals = _rpn_stack_peek(ctxt, 1);
    const auto& value = _rpn_stack_peek(ctxt, 2);

    if (!decimals.isNumber() || !value.isNumber()) {
        return false;
    }
    
    rpn_float_t multiplier = 1.0;
    for (int i = 0; i < round(rpn_float_t(decimals)); ++i) {
        multiplier *= 10.0;
    }
    
    rpn_value result { rpn_float_t(int(rpn_float_t(value) * multiplier + 0.5) / multiplier) };

    _rpn_stack_eat(ctxt, 2);
    rpn_stack_push(ctxt, std::move(result));

    return true;

}

// [a] -> [b], where b is equal to ceil(a)
// stops execution when `a` is not a number
bool _rpn_ceil(rpn_context & ctxt) {
    auto& value = _rpn_stack_peek(ctxt, 1);
    if (!value.isNumber()) {
        return false;
    }

    rpn_value result { ceil(rpn_float_t(value)) };
    _rpn_stack_eat(ctxt, 1);
    rpn_stack_push(ctxt, std::move(result));
    return true;
}

// [a] -> [b], where b is equal to floor(a)
// stops execution when `a` is not a number
bool _rpn_floor(rpn_context & ctxt) {
    auto& value = _rpn_stack_peek(ctxt, 1);
    if (!value.isNumber()) {
        return false;
    }

    rpn_value result { floor(rpn_float_t(value)) };
    _rpn_stack_eat(ctxt, 1);
    rpn_stack_push(ctxt, std::move(result));
    return true;
}

// ----------------------------------------------------------------------------
// Conditionals
// ----------------------------------------------------------------------------

// [a b c] -> [b] when a resolves `true`
// [a b c] -> [c] when a resolves `false`
bool _rpn_ifn(rpn_context & ctxt) {
    const auto c = _rpn_stack_peek(ctxt, 1);
    const auto b = _rpn_stack_peek(ctxt, 2);
    const auto a = _rpn_stack_peek(ctxt, 3);

    _rpn_stack_eat(ctxt, 3);
    rpn_value result { bool(a) ? b : c };
    rpn_stack_push(ctxt, result);

    return true;
}

// [a] -> []
// stops execution when a resolves to `false`
bool _rpn_end(rpn_context & ctxt) {
    const auto value = _rpn_stack_peek(ctxt, 1);
    _rpn_stack_eat(ctxt, 1);
    return bool(value);
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
    const auto b = _rpn_stack_peek(ctxt, 2);
    const auto a = _rpn_stack_peek(ctxt, 3);

    _rpn_stack_eat(ctxt, 3);

    rpn_stack_push(ctxt, c);
    rpn_stack_push(ctxt, a);
    rpn_stack_push(ctxt, b);

    return true;
}

// [a b c] -> [b c a]
bool _rpn_rot(rpn_context & ctxt) {
    const auto c = _rpn_stack_peek(ctxt, 1);
    const auto b = _rpn_stack_peek(ctxt, 2);
    const auto a = _rpn_stack_peek(ctxt, 3);

    _rpn_stack_eat(ctxt, 3);

    rpn_stack_push(ctxt, b); 
    rpn_stack_push(ctxt, c);
    rpn_stack_push(ctxt, a);

    return true;
}

// [a b c] -> [a b]
bool _rpn_drop(rpn_context & ctxt) {
    _rpn_stack_eat(ctxt, 1);
    return true;
}

// [a b c] -> [a b c 3]
bool _rpn_depth(rpn_context & ctxt) {
    rpn_stack_push(ctxt, rpn_value(
        static_cast<rpn_uint_t>(rpn_stack_size(ctxt))
    ));
    return true;
}

// [$var exists] -> [$var]
// stops execution when $var is null
bool _rpn_exists(rpn_context & ctxt) {
    return !ctxt.stack.back().value->isNull();
}

// [a $var] -> [$var]
// $var is persisted and set to the value of a
bool _rpn_assign(rpn_context & ctxt) {
    auto& top = _rpn_stack_peek(ctxt, 1);
    auto& prev = _rpn_stack_peek(ctxt, 2);
    top = prev;
    ctxt.stack.erase(ctxt.stack.end() - 2);
    return true;
}

} // namespace anonymous

// ----------------------------------------------------------------------------
// Functions methods
// ----------------------------------------------------------------------------

rpn_operator::rpn_operator(const char* name, unsigned char argc, rpn_operator_callback_f callback) :
    name(name),
    argc(argc),
    callback(callback)
{}

bool rpn_operator_set(rpn_context & ctxt, const char * name, unsigned char argc, rpn_operator_callback_f callback) {
    ctxt.operators.emplace_back(name, argc, callback);
    return true;
}

bool rpn_operators_clear(rpn_context & ctxt) {
    ctxt.operators.clear();
    return true;
}

bool rpn_operators_init(rpn_context & ctxt) {

    #if RPNLIB_BUILTIN_OPERATORS

    rpn_operator_set(ctxt, "pi", 0, _rpn_pi);
    rpn_operator_set(ctxt, "e", 0, _rpn_e);

    rpn_operator_set(ctxt, "+", 2, _rpn_sum);
    rpn_operator_set(ctxt, "-", 2, _rpn_substract);
    rpn_operator_set(ctxt, "*", 2, _rpn_times);
    rpn_operator_set(ctxt, "/", 2, _rpn_divide);
    rpn_operator_set(ctxt, "mod", 2, _rpn_mod);
    rpn_operator_set(ctxt, "abs", 1, _rpn_abs);

    rpn_operator_set(ctxt, "round", 2, _rpn_round);
    rpn_operator_set(ctxt, "ceil", 1, _rpn_ceil);
    rpn_operator_set(ctxt, "floor", 1, _rpn_floor);
    rpn_operator_set(ctxt, "int", 1, _rpn_floor);

    rpn_operator_set(ctxt, "eq", 2, _rpn_eq);
    rpn_operator_set(ctxt, "ne", 2, _rpn_ne);
    rpn_operator_set(ctxt, "gt", 2, _rpn_gt);
    rpn_operator_set(ctxt, "ge", 2, _rpn_ge);
    rpn_operator_set(ctxt, "lt", 2, _rpn_lt);
    rpn_operator_set(ctxt, "le", 2, _rpn_le);

    rpn_operator_set(ctxt, "cmp", 2, _rpn_cmp);
    rpn_operator_set(ctxt, "cmp3", 3, _rpn_cmp3);
    rpn_operator_set(ctxt, "index", 1, _rpn_index);
    rpn_operator_set(ctxt, "map", 5, _rpn_map);
    rpn_operator_set(ctxt, "constrain", 3, _rpn_constrain);

    rpn_operator_set(ctxt, "and", 2, _rpn_and);
    rpn_operator_set(ctxt, "or", 2, _rpn_or);
    rpn_operator_set(ctxt, "xor", 2, _rpn_xor);
    rpn_operator_set(ctxt, "not", 1, _rpn_not);

    rpn_operator_set(ctxt, "dup", 1, _rpn_dup);
    rpn_operator_set(ctxt, "dup2", 2, _rpn_dup2);
    rpn_operator_set(ctxt, "swap", 2, _rpn_swap);
    rpn_operator_set(ctxt, "rot", 3, _rpn_rot);
    rpn_operator_set(ctxt, "unrot", 3, _rpn_unrot);
    rpn_operator_set(ctxt, "drop", 1, _rpn_drop);
    rpn_operator_set(ctxt, "over", 2, _rpn_over);
    rpn_operator_set(ctxt, "depth", 0, _rpn_depth);

    rpn_operator_set(ctxt, "exists", 1, _rpn_exists);
    rpn_operator_set(ctxt, "=", 2, _rpn_assign);

    rpn_operator_set(ctxt, "ifn", 3, _rpn_ifn);
    rpn_operator_set(ctxt, "end", 1, _rpn_end);

    #ifdef RPNLIB_ADVANCED_MATH
        rpn_operators_fmath_init(ctxt);
    #endif

    #endif // RPNLIB_BUILTIN_OPERATORS

    return true;
}

