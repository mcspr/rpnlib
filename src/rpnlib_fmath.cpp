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

// ----------------------------------------------------------------------------
// Advanced math
// ----------------------------------------------------------------------------

#ifdef RPNLIB_ADVANCED_MATH

bool _rpn_sqrt(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, fs_sqrt(a));
    return true;
}

bool _rpn_log(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    if (0 >= a) {
        rpn_error = RPN_ERROR_UNVALID_ARGUMENT;
        return false;
    }
    rpn_stack_push(ctxt, fs_log(a));
    return true;
}

bool _rpn_log10(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    if (0 >= a) {
        rpn_error = RPN_ERROR_UNVALID_ARGUMENT;
        return false;
    }
    rpn_stack_push(ctxt, fs_log10(a));
    return true;
}

bool _rpn_exp(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, fs_exp(a));
    return true;
}

bool _rpn_fmod(rpn_context & ctxt) {
    float a, b;
    rpn_stack_pop(ctxt, b);
    rpn_stack_pop(ctxt, a);
    if (0 == b) {
        rpn_error = RPN_ERROR_DIVIDE_BY_ZERO;
        return false;
    }
    rpn_stack_push(ctxt, fs_fmod(a, b));
    return true;
}

bool _rpn_pow(rpn_context & ctxt) {
    float a, b;
    rpn_stack_pop(ctxt, b);
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, fs_pow(a, b));
    return true;
}

bool _rpn_cos(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    rpn_stack_push(ctxt, fs_cos(a));
    return true;
}

bool _rpn_sin(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    float cos = fs_cos(a);
    float sin = fs_sqrt(1 - cos * cos);
    rpn_stack_push(ctxt, sin);
    return true;
}

bool _rpn_tan(rpn_context & ctxt) {
    float a;
    rpn_stack_pop(ctxt, a);
    float cos = fs_cos(a);
    if (0 == cos) {
        rpn_error = RPN_ERROR_UNVALID_ARGUMENT;
        return false;
    }
    float sin = fs_sqrt(1 - cos * cos);
    rpn_stack_push(ctxt, sin / cos);
    return true;
}

#endif

