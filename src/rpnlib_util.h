/*

RPNlib

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

#pragma once

#include "rpnlib.h"

struct rpn_decode_errors {

    using callback_f = void (*)(const String&);

    rpn_decode_errors(callback_f callback) :
        callback(callback)
    {}

    void operator ()(rpn_processing_error error);
    void operator ()(rpn_operator_error error);
    void operator ()(rpn_value_error error);
    void operator ()(int code);

    callback_f callback;

};
