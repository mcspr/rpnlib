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

#include <cstdint>

#ifndef RPNLIB_INT_TYPE
#define RPNLIB_INT_TYPE int32_t
#endif

#ifndef RPNLIB_UINT_TYPE
#define RPNLIB_UINT_TYPE uint32_t
#endif

#ifndef RPNLIB_FLOAT_TYPE
#define RPNLIB_FLOAT_TYPE double
#endif

#ifndef RPNLIB_EXPRESSION_BUFFER_SIZE
#define RPNLIB_EXPRESSION_BUFFER_SIZE  256
#endif

#ifndef RPNLIB_BUILTIN_OPERATORS
#define RPNLIB_BUILTIN_OPERATORS    1
#endif
