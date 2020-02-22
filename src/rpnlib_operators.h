/*

RPNlib

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

Some refactoring by Maxim Prokhorov <prokhorov dot max at outlook dot com>

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

#pragma once

#define RPN_CONST_PI    3.141593
#define RPN_CONST_E     2.178282

bool _rpn_pi(rpn_context &);
bool _rpn_e(rpn_context &);
bool _rpn_sum(rpn_context &);
bool _rpn_substract(rpn_context &);
bool _rpn_times(rpn_context &);
bool _rpn_divide(rpn_context &);
bool _rpn_mod(rpn_context &);
bool _rpn_abs(rpn_context &);
bool _rpn_sqrt(rpn_context &);
bool _rpn_log(rpn_context &);
bool _rpn_log10(rpn_context &);
bool _rpn_exp(rpn_context &);
bool _rpn_fmod(rpn_context &);
bool _rpn_pow(rpn_context &);
bool _rpn_cos(rpn_context &);
bool _rpn_sin(rpn_context &);
bool _rpn_tan(rpn_context &);
bool _rpn_eq(rpn_context &);
bool _rpn_ne(rpn_context &);
bool _rpn_gt(rpn_context &);
bool _rpn_ge(rpn_context &);
bool _rpn_lt(rpn_context &);
bool _rpn_le(rpn_context &);
bool _rpn_cmp(rpn_context &);
bool _rpn_cmp3(rpn_context &);
bool _rpn_index(rpn_context &);
bool _rpn_map(rpn_context &);
bool _rpn_constrain(rpn_context &);
bool _rpn_and(rpn_context &);
bool _rpn_or(rpn_context &);
bool _rpn_xor(rpn_context &);
bool _rpn_not(rpn_context &);
bool _rpn_round(rpn_context &);
bool _rpn_ceil(rpn_context &);
bool _rpn_floor(rpn_context &);
bool _rpn_ifn(rpn_context &);
bool _rpn_end(rpn_context &);
bool _rpn_dup(rpn_context &);
bool _rpn_dup2(rpn_context &);
bool _rpn_over(rpn_context &);
bool _rpn_swap(rpn_context &);
bool _rpn_unrot(rpn_context &);
bool _rpn_rot(rpn_context &);
bool _rpn_drop(rpn_context &);
bool _rpn_depth(rpn_context &);
bool _rpn_exists(rpn_context &);
bool _rpn_assign(rpn_context &);
