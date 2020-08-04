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
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    rpn_value result { static_cast<rpn_float>(fs_sqrt(conversion.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_log(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    if (static_cast<rpn_float>(0.0) >= conversion.value()) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float(fs_log(conversion.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_log10(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    if (static_cast<rpn_float>(0.0) >= conversion.value()) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float(fs_log10(conversion.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_exp(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    rpn_value result { rpn_float(fs_exp(conversion.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_fmod(rpn_context & ctxt) {
    auto b = rpn_stack_pop(ctxt);
    auto a = rpn_stack_pop(ctxt);

    auto convert_a = a.checkedToFloat();
    if (!convert_a.ok()) {
        return convert_a.error();
    }

    auto convert_b = b.checkedToFloat();
    if (!convert_b.ok()) {
        return convert_b.error();
    }

    if (static_cast<rpn_float>(0.0) == convert_b.value()) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { static_cast<rpn_float>(fs_fmod(convert_a.value(), convert_b.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_pow(rpn_context & ctxt) {
    auto b = rpn_stack_pop(ctxt);
    auto a = rpn_stack_pop(ctxt);

    auto convert_a = a.checkedToFloat();
    if (!convert_a.ok()) {
        return convert_a.error();
    }

    auto convert_b = b.checkedToFloat();
    if (!convert_b.ok()) {
        return convert_b.error();
    }

    rpn_value result { static_cast<rpn_float>(fs_pow(convert_a.value(), convert_b.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_cos(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    rpn_value result { rpn_float(fs_cos(conversion.value())) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_sin(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    auto cos = fs_cos(conversion.value());

    rpn_value result { rpn_float(fs_sqrt(1.0 - cos * cos)) };
    rpn_stack_push(ctxt, std::move(result));

    return 0;
}

rpn_error _rpn_tan(rpn_context & ctxt) {
    auto a = rpn_stack_pop(ctxt);
    auto conversion = a.checkedToFloat();
    if (!conversion.ok()) {
        return conversion.error();
    }

    auto cos = fs_cos(conversion.value());
    if (0.0 == cos) {
        return rpn_operator_error::InvalidArgument;
    }

    rpn_value result { rpn_float(fs_sqrt(1.0 - cos * cos) / cos) };
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

