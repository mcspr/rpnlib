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

#pragma once

#include <Arduino.h>
#include <core_version.h>

#include <cmath>
#include <cstdlib>

// Clean-up after Arduino.h legacy overrides / re-implementations of math.h
#ifdef round
#undef round
#endif

#ifdef abs
#undef abs
#endif

// Note: we could implement this as type trait, but there are much more factors than this specific version check. Future versions promise to fix this, so that's good enough.
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)

inline float rpnlib_round(float val) {
    return roundf(val);
}

inline double rpnlib_round(double val) {
    return round(val);
}

inline long double rpnlib_round(long double val) {
    return roundl(val);
}

inline float rpnlib_abs(float val) {
    return fabsf(val);
}

inline double rpnlib_abs(double val) {
    return fabs(val);
}

inline long double rpnlib_abs(long double val) {
    return fabsl(val);
}

inline int rpnlib_abs(int val) {
    return abs(val);
}

inline long int rpnlib_abs(long int val) {
    return labs(val);
}

inline long long int rpnlib_abs(long long int val) {
    return llabs(val);
}

#else

template <typename T>
inline T rpnlib_round(T val) {
    return std::round(val);
}

template <typename T>
inline T rpnlib_abs(T val) {
    return std::abs(val);
}

#endif // if defined ARDUINO_ESP8266_RELEASE_2_3_0
