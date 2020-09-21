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

#ifndef rpnlib_h
#define rpnlib_h

#include <Arduino.h>

#include "rpnlib_config.h"
#include "rpnlib_error.h"

#include <vector>

using rpn_int = RPNLIB_INT_TYPE;
using rpn_float = RPNLIB_FLOAT_TYPE;
using rpn_uint = RPNLIB_UINT_TYPE;

struct rpn_context;

// ----------------------------------------------------------------------------

#include "rpnlib_value.h"
#include "rpnlib_operators.h"
#include "rpnlib_variable.h"
#include "rpnlib_stack.h"

// XXX: In theory, this could be Arduino String class. However, String::concat(cstring, length) is hidden by default.
// We *could* easily make it public via subclassing, however some implementations do resort to using strcpy, completely ignoring 'length' param:
// - https://github.com/bxparks/UnixHostDuino/blob/8564fdde95c1a41b01d8dd87e22cf01aa24332e5/WString.cpp#L264-L272
// - https://github.com/arduino/ArduinoCore-API/blob/45e4e5aba6ad1a5b67026b686099cc0fce437cdc/api/String.cpp#L267-L275
// Thus, making it harder to streamline the testing :/ As the hardware version will work just fine b/c it uses either memmove or memcpy:
// - https://github.com/esp8266/Arduino/blob/fc2426a5e96525a90b9b97f3bd9b679ed904b141/cores/esp8266/WString.cpp#L332
// - https://github.com/espressif/arduino-esp32/blob/4d98cea085d619bed7026b37071bd8402a485d95/cores/esp32/WString.cpp#L335-L338
// XXX: std::string vs. String, stdlib's option seems more reasonable. we don't care about sso stuff, but *do* care about code bloat because of multiple string implementations

struct rpn_input_buffer {
    constexpr static size_t Size = RPNLIB_EXPRESSION_BUFFER_SIZE;

    bool ok() const {
        return !_overflow;
    }

    const char* c_str() const;
    size_t length() const;

    bool operator==(const String& other) const;

    rpn_input_buffer& operator+=(char c);
    rpn_input_buffer& write(const char* data, size_t data_length);
    rpn_input_buffer& assign(const char* data, size_t data_length);
    void reset();

    private:

    char _buffer[Size] __attribute__((aligned(4))) { 0 };
    size_t _length { 0ul };
    bool _overflow { false };
};

struct rpn_context {
    using debug_callback_type = void(*)(rpn_context &, const char *);
    using operators_type = std::vector<rpn_operator>;
    using variables_type = std::vector<rpn_variable>;

    debug_callback_type debug_callback;

    rpn_input_buffer input_buffer;
    rpn_error error;

    variables_type variables;
    operators_type operators;
    rpn_nested_stack stack;
};

// ----------------------------------------------------------------------------

#include "rpnlib_util.h"

bool rpn_process(rpn_context &, const char *, bool variable_must_exist = false);
bool rpn_init(rpn_context &);
bool rpn_clear(rpn_context &);

bool rpn_debug(rpn_context &, rpn_context::debug_callback_type);

// ----------------------------------------------------------------------------

#endif // rpnlib_h
