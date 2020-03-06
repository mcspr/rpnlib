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
#include "rpnlib_operators.h"

extern "C" {
    #include "fs_math.h"
}

#ifdef RPNLIB_ADVANCED_MATH

// anonymous namespace binds all of the functions below to this compilation unit
// this has the same effect as if these functions were `static`
namespace {

// ----------------------------------------------------------------------------
// Advanced math
// ----------------------------------------------------------------------------

bool _rpn_sqrt(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, rpn_float_t(fs_sqrt(a)));
    return true;
}

bool _rpn_log(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    if (0 >= a) {
        rpn_error = RPN_ERROR_INVALID_ARGUMENT;
        return false;
    }
    rpn_stack_push(ctxt, rpn_float_t(fs_log(a)));
    return true;
}

bool _rpn_log10(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    if (0.0L >= a) {
        rpn_error = RPN_ERROR_INVALID_ARGUMENT;
        return false;
    }
    rpn_stack_push(ctxt, rpn_float_t(fs_log10(a)));
    return true;
}

bool _rpn_exp(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, rpn_float_t(fs_exp(a)));
    return true;
}

bool _rpn_fmod(rpn_context & ctxt) {
    rpn_float_t a, b;
    rpn_stack_pop(ctxt, b);
    rpn_stack_pop(ctxt, a);
    if (0.0L == b) {
        rpn_error = RPN_ERROR_DIVIDE_BY_ZERO;
        return false;
    }
    rpn_stack_push(ctxt, rpn_float_t(fs_fmod(a, b)));
    return true;
}

bool _rpn_pow(rpn_context & ctxt) {
    rpn_float_t a, b;
    rpn_stack_pop(ctxt, b);
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, rpn_float_t(fs_pow(a, b)));
    return true;
}

bool _rpn_cos(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, rpn_float_t(fs_cos(a)));
    return true;
}

bool _rpn_sin(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    const double cos = fs_cos(a);
    const double sin = fs_sqrt(1.0L - cos * cos);
    rpn_stack_push(ctxt, rpn_float_t(sin));
    return true;
}

bool _rpn_tan(rpn_context & ctxt) {
    rpn_float_t a;
    rpn_stack_pop(ctxt, a);
    const double cos = fs_cos(a);
    if (0.0L == cos) {
        rpn_error = RPN_ERROR_INVALID_ARGUMENT;
        return false;
    }
    const double sin = fs_sqrt(1.0L - cos * cos);
    rpn_stack_push(ctxt, rpn_float_t(sin / cos));
    return true;
}

} // namespace anonymous

bool rpn_operators_fmath_init(rpn_context & ctxt) {
    rpn_operator_set(ctxt, "sqrt", 1, _rpn_sqrt);
    rpn_operator_set(ctxt, "log", 1, _rpn_log);
    rpn_operator_set(ctxt, "log10", 1, _rpn_log10);
    rpn_operator_set(ctxt, "exp", 1, _rpn_exp);
    rpn_operator_set(ctxt, "fmod", 2, _rpn_fmod);
    rpn_operator_set(ctxt, "pow", 2, _rpn_pow);
    rpn_operator_set(ctxt, "cos", 1, _rpn_cos);
    rpn_operator_set(ctxt, "sin", 1, _rpn_sin);
    rpn_operator_set(ctxt, "tan", 1, _rpn_tan);
    return true;
}

#endif // ifdef RPNLIB_ADVANCED_MATH

