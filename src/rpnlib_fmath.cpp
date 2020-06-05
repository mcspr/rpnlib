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

rpn_error _rpn_sqrt(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    rpn_float_t value { fs_sqrt(a.toFloat()) };
    rpn_value result { value };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_log(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    auto a_val { a.toFloat() };
    if (0 >= a_val) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float_t(fs_log(a_val)) };
    rpn_stack_push(ctxt, result);
    return 0;
}

rpn_error _rpn_log10(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    auto a_val { a.toFloat() };
    if (0.0 >= a_val) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float_t(fs_log10(a_val)) };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_exp(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    auto a_val { a.toFloat() };
    rpn_value result { rpn_float_t(fs_exp(a_val)) };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_fmod(rpn_context & ctxt) {
    rpn_value a;
    rpn_value b;

    rpn_stack_pop(ctxt, a);
    rpn_stack_pop(ctxt, b);

    auto a_val { a.toFloat() };
    auto b_val { b.toFloat() };
    if (0.0 == b_val) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { fs_fmod(a_val, b_val) };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_pow(rpn_context & ctxt) {
    rpn_value a;
    rpn_value b;

    rpn_stack_pop(ctxt, a);
    rpn_stack_pop(ctxt, b);

    rpn_value result { fs_pow(a.toFloat(), b.toFloat()) };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_cos(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    rpn_value result { rpn_float_t(fs_cos(a.toFloat())) };
    rpn_stack_push(ctxt, result);

    return 0;
}

rpn_error _rpn_sin(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    auto cos = fs_cos(a.toFloat());

    rpn_value result { rpn_float_t(fs_sqrt(1.0 - cos * cos)) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_tan(rpn_context & ctxt) {
    rpn_value a;
    rpn_stack_pop(ctxt, a);

    auto cos = fs_cos(a.toFloat());
    if (0.0 == cos) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float_t(fs_sqrt(1.0 - cos * cos) / cos) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
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

