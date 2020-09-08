/*

RPNlib

PlatformIO Unit Tests

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

#include <Arduino.h>
#include <unity.h>
#include <unity_internals.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

// XXX: we still need printf for format
#define __STDC_FORMAT_MACROS
#include <cinttypes>

// Detect when running on host. Either in `pio test` or via `examples/host`
#if defined(UNIX_HOST_DUINO) || HOST_MOCK
#define RPNLIB_PIOTEST_HOST_TEST 1
#else
#define RPNLIB_PIOTEST_HOST_TEST 0
#endif

#include <rpnlib.h>

// -----------------------------------------------------------------------------
// Helper methods
// -----------------------------------------------------------------------------

// TODO: Integer literals are int, while rpn_int could be a different type.
//       Most of the time we want to use rpn_values(...) helper or rpn_value ctor
//       to compare with something that had been parsed. But, right now we need to
//       either explicitly cast the literal:
//           `static_cast<rpn_int>(12345)`
//           `static_cast<rpn_uint>(67890u)`
//       Or, specify parameter pack types:
//           `rpn_values<rpn_int, rpn_int>(123, 456)`
//           `rpn_values<rpn_uint, rpn_int>(111u, 222)`
//
//       Some possible options:
//       - enforce automatic conversion for rpn_values() inputs
//       - provide properly typed array / structure instead of arguments
//       - ...use intmax_t in rpn_value?
//

// Notice that parameter pack Args can contain diffeent types
template <typename... Args>
std::array<rpn_value, sizeof...(Args)> rpn_values(Args... args) {
    return {rpn_value(args)...};
}

// TODO: perhaps this could be in rpnlib utils?
const char* explain_type(rpn_value::Type type) {
    switch (type) {
    case rpn_value::Type::Integer:
        return "Integer";
    case rpn_value::Type::Unsigned:
        return "Unsigned";
    case rpn_value::Type::Float:
        return "Float";
    case rpn_value::Type::String:
        return "String";
    case rpn_value::Type::Boolean:
        return "Boolean";
    case rpn_value::Type::Null:
        return "Null";
    case rpn_value::Type::Error:
        return "Error";
    default:
        return "(unknown)";
    }
}

int explain_contents(char* output, size_t output_size, const rpn_value& value) {
    switch (value.type) {
    case rpn_value::Type::Integer:
        return snprintf(output, output_size - 1, "%" PRIdMAX, static_cast<std::intmax_t>(value.toInt()));
    case rpn_value::Type::Unsigned:
        return snprintf(output, output_size - 1, "%" PRIuMAX, static_cast<std::uintmax_t>(value.toUint()));
    case rpn_value::Type::Float:
        return snprintf(output, output_size - 1, "%f", value.toFloat());
    case rpn_value::Type::String:
        return snprintf(output, output_size - 1, "\"%s\"", value.toString().c_str());
    case rpn_value::Type::Boolean:
        return snprintf(output, output_size - 1, "%s", value.toBoolean() ? "true" : "false");
    case rpn_value::Type::Null:
        return snprintf(output, output_size - 1, "null");
    case rpn_value::Type::Error:
        return snprintf(output, output_size - 1, "error (this should not happen)");
    default:
        return snprintf(output, output_size - 1, "unknown (this should not happen!!!)");
    }
}

template <typename T>
void _stack_compare(rpn_context& ctxt, T expected, int line) {
    UNITY_TEST_ASSERT_EQUAL_UINT(expected.size(), rpn_stack_size(ctxt),
            line, "Stack size does not match the expected value");

    auto index = rpn_stack_size(ctxt) - 1;

    // 'expected' is arranged as `begin()` == bottom and `end() - 1` == top
    std::vector<rpn_value> stack_values;
    while (rpn_stack_size(ctxt)) {
        stack_values.push_back(rpn_stack_pop(ctxt));
    }
    std::reverse(stack_values.begin(), stack_values.end());

    char buffer[512] = {0};

    // start checking from the top
    auto expect = expected.rbegin();
    auto stack = stack_values.rbegin();
    while ((expect != expected.rend()) && (stack != stack_values.rend())) {
        if (!(*expect).is((*stack).type)) {
            sprintf(buffer, "Index %zu TYPE MISMATCH : Expected %s, Got %s",
                index, explain_type((*expect).type), explain_type((*stack).type));
            UNITY_TEST_FAIL(line, buffer);
        }

        if ((*expect) != (*stack)) {
            auto offset = sprintf(buffer, "Index %zu VALUE MISMATCH : Expected ", index);
            offset += explain_contents(buffer + offset, sizeof(buffer) - offset, (*expect));
            offset += sprintf(buffer + offset, ", Got ");
            offset += explain_contents(buffer + offset, sizeof(buffer) - offset, (*stack));
            UNITY_TEST_FAIL(line, buffer);
        }

        --index;
        ++expect;
        ++stack;
    }
}

template <typename T>
void _run_and_compare(rpn_context & ctxt, const char* command, T expected, int line) {
    UnityMessage(command, line);

    if (!rpn_process(ctxt, command)) {
        String message("rpn_process() failed with \"");
        rpn_handle_error(ctxt.error, rpn_decode_errors([&message](const String& decoded) {
            message += decoded;
        }));
        message += "\" at position ";
        message += ctxt.error.position;
        UNITY_TEST_FAIL(line, message.c_str());
    }

    UNITY_TEST_ASSERT_EQUAL_INT(0, ctxt.error.code,
            line, "There should be no error code set after rpn_process() returns true");

    _stack_compare(ctxt, expected, line);
}

template <typename T>
void _run_and_compare(const char* command, T expected, int line) {
    rpn_context ctxt;
    UNITY_TEST_ASSERT(rpn_init(ctxt), line, "rpn_init() should return true");
    _run_and_compare(ctxt, command, expected, line);
}

void _run_and_error(rpn_context& ctxt, const char* command, rpn_error error, const char* message, int line) {
    UnityMessage(command, line);

    char buffer[256];
    if (!rpn_process(ctxt, command)) {
        sprintf(buffer, "Expected %s, Got {category %d, code %d at %zu}",
            message, static_cast<int>(ctxt.error.category), ctxt.error.code, ctxt.error.position);
        UNITY_TEST_ASSERT((error == ctxt.error), line, buffer);
        return;
    }

    sprintf(buffer, "Expected to fail with %s", message);
    UNITY_TEST_FAIL(line, buffer);
}

void _run_and_error(const char * command, rpn_error error, const char* message, int line) {
    rpn_context ctxt;
    UNITY_TEST_ASSERT(rpn_init(ctxt), line, nullptr);
    _run_and_error(ctxt, command, error, message, line);
}

// Allow unity tests to reflect the real line number, not the line number of the helper function

#define _RUN_TEST_STRINGIFY(X) #X
#define RUN_TEST_STRINGIFY(X) _RUN_TEST_STRINGIFY(X)

#define stack_compare(context, expected) \
    _stack_compare(context, expected, __LINE__)

#define run_and_compare(command, expected) \
    _run_and_compare(command, expected, __LINE__)

#define run_and_compare_ctx(context, command, expected) \
    _run_and_compare(context, command, expected, __LINE__)

#define run_and_error(command, error) \
    _run_and_error(command, error, RUN_TEST_STRINGIFY(error), __LINE__)

#define run_and_error_ctx(ctx, command, error) \
    _run_and_error(ctx, command, error, RUN_TEST_STRINGIFY(error), __LINE__)

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_rpn_value() {
    rpn_value rpn_default;
    TEST_ASSERT(rpn_default.isNull());
    TEST_ASSERT_EQUAL(false, rpn_default.toInt());
    TEST_ASSERT_EQUAL(0, rpn_default.toInt());
    TEST_ASSERT_EQUAL(0UL, rpn_default.toUint());
    TEST_ASSERT_EQUAL_FLOAT(0.0, rpn_default.toFloat());

    rpn_value rpn_bool { true };
    TEST_ASSERT(rpn_bool.isBoolean());
    TEST_ASSERT(rpn_bool.toBoolean());

    rpn_value as_int { static_cast<rpn_int>(2) };
    TEST_ASSERT(as_int.isInt());
    TEST_ASSERT_EQUAL(2, as_int.toInt());

    rpn_value as_uint { static_cast<rpn_uint>(3) };
    TEST_ASSERT(as_uint.isUint());
    TEST_ASSERT_EQUAL(3UL, as_uint.toUint());

    rpn_value as_float { static_cast<rpn_float>(1.0) };
    TEST_ASSERT(as_float.isFloat());
    TEST_ASSERT_EQUAL_FLOAT(1.0, as_float.toFloat());

    rpn_value as_string { "12345" };
    TEST_ASSERT(as_string.isString());
    TEST_ASSERT_EQUAL_STRING("12345", as_string.toString().c_str());
}

// Ensures default configuration works, but we may encounter different combinations...
void test_conversions() {

    {
        rpn_value value{};
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT_FALSE((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { rpn_value_error::NotAnError };
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT_FALSE((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { std::numeric_limits<rpn_int>::min() };
        TEST_ASSERT((value.checkedToInt()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT_EQUAL(rpn_value_error::OutOfRangeConversion, value.checkedToUint().error());
    }

    {
        rpn_value value { std::numeric_limits<rpn_int>::max() };
        TEST_ASSERT((value.checkedToInt()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
        TEST_ASSERT((value.checkedToUint()).ok());
    }

    {
        rpn_value value { std::numeric_limits<rpn_uint>::max() };
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT((value.checkedToUint()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { std::numeric_limits<rpn_uint>::min() };
        TEST_ASSERT((value.checkedToInt()).ok());
        TEST_ASSERT((value.checkedToUint()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { std::numeric_limits<rpn_float>::max() };
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { std::numeric_limits<rpn_float>::lowest() };
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT((value.checkedToFloat()).ok());
    }

    {
        rpn_value value { "1234567890" };
        TEST_ASSERT_FALSE((value.checkedToInt()).ok());
        TEST_ASSERT_FALSE((value.checkedToUint()).ok());
        TEST_ASSERT_FALSE((value.checkedToFloat()).ok());
    }

}

// TODO: also check integer operations
// TODO: make sure we handle math processing errors

void test_math() {
    run_and_compare("-5 -2 -1 * * abs", rpn_values(10.0));
    run_and_compare("5 2 * 3 + 5 mod", rpn_values(3.0));
}

void test_math_advanced() {
#ifdef RPNLIB_ADVANCED_MATH
    run_and_compare("10 2 pow sqrt log10 floor", rpn_values(1.0));
#else
    TEST_IGNORE_MESSAGE("fmath is disabled");
#endif
}

// TODO: we can't insert integers without creating rpn_value manually
//       every number parsed from expression will be floating point
void test_math_uint() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    const auto first_value = static_cast<rpn_uint>(12345);
    const auto second_value = static_cast<rpn_uint>(56789);

    rpn_value first(first_value);
    rpn_value second(second_value);

    TEST_ASSERT(first.isUint());
    TEST_ASSERT_EQUAL(first_value, first.toUint());

    TEST_ASSERT(second.isUint());
    TEST_ASSERT_EQUAL(second_value, second.toUint());

    auto check_expression = [&](const char* expression, rpn_uint result) {
        TEST_ASSERT(rpn_stack_push(ctxt, first));
        TEST_ASSERT(rpn_stack_push(ctxt, second));
        TEST_ASSERT_EQUAL(2, rpn_stack_size(ctxt));

        TEST_ASSERT(rpn_process(ctxt, expression));
        TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

        rpn_value output(rpn_stack_pop(ctxt));
        TEST_ASSERT_EQUAL(result, output.toUint());
    };

    check_expression("+", first_value + second_value);
    check_expression("-", first_value - second_value);
    check_expression("*", first_value * second_value);
    check_expression("/", first_value / second_value);
}

void test_math_int() {

    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    const auto first_value = static_cast<rpn_int>(50);
    const auto second_value = static_cast<rpn_int>(25);

    rpn_value first(first_value);
    rpn_value second(second_value);

    TEST_ASSERT(first.isInt());
    TEST_ASSERT_EQUAL(first_value, first.toInt());

    TEST_ASSERT(second.isInt());
    TEST_ASSERT_EQUAL(second_value, second.toInt());

    auto check_expression = [&](const char* expression, rpn_int result) {
        TEST_ASSERT(rpn_stack_push(ctxt, first));
        TEST_ASSERT(rpn_stack_push(ctxt, second));
        TEST_ASSERT_EQUAL(2, rpn_stack_size(ctxt));

        TEST_ASSERT(rpn_process(ctxt, expression));
        TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

        rpn_value output(rpn_stack_pop(ctxt));
        TEST_ASSERT_EQUAL(result, output.toInt());
    };

    check_expression("+", first_value + second_value);
    check_expression("-", first_value - second_value);
    check_expression("*", first_value * second_value);
    check_expression("/", first_value / second_value);

}

void test_trig() {
#ifdef RPNLIB_ADVANCED_MATH
    run_and_compare("pi 4 / cos 2 sqrt * 3 round", rpn_values(1.000));
#else
    TEST_IGNORE_MESSAGE("fmath is disabled");
#endif
}

void test_cast() {
    run_and_compare("pi 2 round pi 4 round 1.1 floor 1.1 ceil",
            rpn_values(3.14, 3.1416, 1.0, 2.0));
}

void test_cmp() {
    run_and_compare("18 24 cmp", rpn_values<rpn_int>(-1));
    run_and_compare("24 18 cmp", rpn_values<rpn_int>(1));
    run_and_compare("18 18 cmp", rpn_values<rpn_int>(0));

    run_and_compare("13 18 24 cmp3", rpn_values<rpn_int>(-1));
    run_and_compare("18 18 24 cmp3", rpn_values<rpn_int>(0));
    run_and_compare("25 18 24 cmp3", rpn_values<rpn_int>(1));
}

void test_index() {
    run_and_compare("2 10 20 30 40 50 5 index", rpn_values(30.0));
    run_and_compare("0 1 1 index", rpn_values(1.0));
    run_and_compare("-5 10 20 30 40 50 5 index", rpn_values(10.0));

    run_and_error("-6 10 20 30 40 50 5 index", rpn_operator_error::InvalidArgument);
    run_and_error("5 10 20 30 40 50 5 index", rpn_operator_error::InvalidArgument);
    run_and_error("0 0 index", rpn_operator_error::InvalidArgument);

    run_and_error("0 index", rpn_operator_error::InvalidArgument);
    run_and_error("index", rpn_operator_error::ArgumentCountMismatch);
}

void test_map() {
    run_and_compare("256 0 1024 0 100 map", rpn_values(25.0));
    run_and_compare("1 0 100 0 1000 map", rpn_values(10.0));
}

void test_constrain() {
    run_and_compare("16 10 15 constrain", rpn_values(15.0));
    run_and_compare("9 10 15 constrain", rpn_values(10.0));
    run_and_compare("13 10 15 constrain", rpn_values(13.0));
}

void test_conditionals() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "1 2 eq end \"test\""));
    TEST_ASSERT_EQUAL(0, rpn_stack_size(ctxt));

    run_and_compare("1 2 3 ifn", rpn_values(2.0));
    run_and_compare("true 4 5 ifn", rpn_values(4.0));
    run_and_compare("false 6 7 ifn", rpn_values(7.0));
    run_and_compare("4 end 1 2 3 ifn", rpn_values(2.0));
}

void test_stack() {
    run_and_compare("1 3 dup unrot swap - *", rpn_values(6.0));
    run_and_compare("1 2 3 rot", rpn_values(2.0, 3.0, 1.0));
    run_and_compare("2 3 1 unrot", rpn_values(1.0, 2.0, 3.0));
    run_and_compare("1 2 3 rot unrot", rpn_values(1.0, 2.0, 3.0));
    run_and_compare("1 2 3 4 5 drop", rpn_values(1.0, 2.0, 3.0, 4.0));
    run_and_compare("1 drop", rpn_values());
    run_and_compare("1 2 over", rpn_values(1.0, 2.0, 1.0));
    run_and_compare("2 1 over", rpn_values(2.0, 1.0, 2.0));
    run_and_compare("1 2 3 4 5 6 7 8 9 depth", rpn_values(
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, static_cast<rpn_uint>(9)));
}

void test_logic() {
    run_and_compare("100 100 eq", rpn_values(true));
    run_and_compare("1 100 eq", rpn_values(false));
    run_and_compare("1 true eq", rpn_values(true));
    run_and_compare("0 false eq", rpn_values(true));
    run_and_compare("100 100 ne", rpn_values(false));
    run_and_compare("100 1 ne", rpn_values(true));
    run_and_compare("2 1 gt", rpn_values(true));
    run_and_compare("1 1 gt", rpn_values(false));
    run_and_compare("100 1 ge", rpn_values(true));
    run_and_compare("100 100 ge", rpn_values(true));
    run_and_compare("100 101 ge", rpn_values(false));
    run_and_compare("1 101 lt", rpn_values(true));
    run_and_compare("2 1 lt", rpn_values(false));
    run_and_compare("2 1 le", rpn_values(false));
    run_and_compare("2 2 le", rpn_values(true));
    run_and_compare("1 2 le", rpn_values(true));
    run_and_compare("1 1 eq 1 1 ne 2 1 gt 2 1 lt",
            rpn_values(true, false, true, false));
}

void test_boolean() {
    // general tokens when converted to boolean (and, or, xor, not)
    run_and_compare("2 2 and", rpn_values(true));
    run_and_compare("false 2 and", rpn_values(false));
    run_and_compare("false false and", rpn_values(false));
    run_and_compare("true false and", rpn_values(false));
    run_and_compare("true 0 and", rpn_values(false));
    run_and_compare("true 1 and", rpn_values(true));
    run_and_compare("true true and", rpn_values(true));

    run_and_compare("2 2 or", rpn_values(true));
    run_and_compare("false 2 or", rpn_values(true));
    run_and_compare("false false or", rpn_values(false));
    run_and_compare("true false or", rpn_values(true));
    run_and_compare("true 0 or", rpn_values(true));
    run_and_compare("true 1 or", rpn_values(true));
    run_and_compare("true true or", rpn_values(true));

    // strings are also convertible to boolean based on length
    {
        run_and_compare("\"\" true and", rpn_values(false));
        run_and_compare("\"\" \"\" and", rpn_values(false));
        run_and_compare("\"\" \"\" or", rpn_values(false));
        run_and_compare("\"\" \"not empty\" or", rpn_values(true));
        run_and_compare("\"not empty\" \"not empty, again\" and", rpn_values(true));
    }

    // integer values have the same semantics as float
    {
        rpn_context ctxt;
        TEST_ASSERT(rpn_init(ctxt));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(12345))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(0))));
        run_and_compare_ctx(ctxt, "and", rpn_values(false));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(12345))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(67890))));
        run_and_compare_ctx(ctxt, "and", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(1))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(1))));
        run_and_compare_ctx(ctxt, "or", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(0))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(1))));
        run_and_compare_ctx(ctxt, "or", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(0))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(0))));
        run_and_compare_ctx(ctxt, "or", rpn_values(false));

    }

    {
        rpn_context ctxt;
        TEST_ASSERT(rpn_init(ctxt));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(12345))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(56789))));
        run_and_compare_ctx(ctxt, "and", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(12345))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(67890))));
        run_and_compare_ctx(ctxt, "and", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(0))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(1))));
        run_and_compare_ctx(ctxt, "or", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(1))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(1))));
        run_and_compare_ctx(ctxt, "or", rpn_values(true));

        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(0))));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(0))));
        run_and_compare_ctx(ctxt, "or", rpn_values(false));
    }

    run_and_compare("2 0 and 2 0 or 2 0 xor 1 not",
            rpn_values(false, true, true, false));
}

void test_variable() {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));

    // should not be able to set impossible to parse variable name
    TEST_ASSERT_FALSE(rpn_variable_set(ctxt, "tmp value", rpn_value { 52.0 }));

    // can access variable that was set externally
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "tmp", rpn_value { 25.0 }));
    run_and_compare_ctx(ctxt, "$tmp 5 /", rpn_values(5.0));

    // cannot do operations with undefined variable
    run_and_error_ctx(ctxt, "25 &unknown +", rpn_value_error::IsNull);
    TEST_ASSERT(rpn_stack_clear(ctxt));

    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));

    // should properly 'swap' the stack, not the value reference for the variable
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 100.0 }));
    run_and_compare_ctx(ctxt, "&var", rpn_values(100.0));
    run_and_compare_ctx(ctxt, "&var 1 swap =", rpn_values(1.0));
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    // should properly rotate ('rot', 'unrot') the stack, similar rule applies
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 200.0 }));
    run_and_compare_ctx(ctxt, "1 &var 300 unrot = -", rpn_values(299.0));
    TEST_ASSERT_EQUAL_FLOAT(1.0, rpn_variable_get(ctxt, "var").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 400.0 }));
    run_and_compare_ctx(ctxt, "&var 300 2 rot = -", rpn_values(298.0));
    TEST_ASSERT_EQUAL_FLOAT(2.0, rpn_variable_get(ctxt, "var").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 20.0 }));
    run_and_compare_ctx(ctxt, "1 10 &var rot unrot = +", rpn_values(11.0));
    TEST_ASSERT_EQUAL_FLOAT(10.0, rpn_variable_get(ctxt, "var").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    // should properly duplicate the stack element ('over') and push it to the top
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 12345.0 }));
    run_and_compare_ctx(ctxt, "&var 54321 over =", rpn_values(54321.0, 54321.0));
    TEST_ASSERT_EQUAL_FLOAT(54321.0, rpn_variable_get(ctxt, "var").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    // should duplicate reference instead of the value itself
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 1000.0 }));
    run_and_compare_ctx(ctxt, "&var dup swap dup2 rot swap + + swap =", rpn_values(3000.0));
    TEST_ASSERT_EQUAL_FLOAT(3000.0, rpn_variable_get(ctxt, "var").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

    // should keep reference instead of the value itself
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "foo", rpn_value { 1.0 }));
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "bar", rpn_value { 2.0 }));
    run_and_compare_ctx(ctxt, "3.0 false &foo &bar ifn =", rpn_values(3.0));
    TEST_ASSERT_EQUAL_FLOAT(3.0, rpn_variable_get(ctxt, "bar").toFloat());
    TEST_ASSERT_EQUAL_FLOAT(1.0, rpn_variable_get(ctxt, "foo").toFloat());
    TEST_ASSERT(rpn_stack_clear(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));

}

void test_variable_operator() {

    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    // ensure basic assignment works
    run_and_compare_ctx(ctxt, "25 &tmp =", rpn_values(25.0));

    {
        rpn_value value;
        TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "tmp", value));
        TEST_ASSERT_EQUAL_FLOAT(25.0, value.toFloat());
    }

    // ensure we don't change the underlying value after dereference
    run_and_error_ctx(ctxt, "20 &tmp deref =", rpn_operator_error::InvalidType);

    {
        rpn_value value;
        TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "tmp", value));
        TEST_ASSERT_EQUAL_FLOAT(25.0, value.toFloat());
    }

    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));

}

void test_variable_cleanup() {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "12.3 &tmp ="));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(12.3, value.toFloat());
    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));

    TEST_ASSERT_TRUE(rpn_process(ctxt, "&tmp exists"));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "null &tmp ="));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "&tmp exists"));
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));

    TEST_ASSERT_TRUE(rpn_process(ctxt, "&tmp"));
    TEST_ASSERT_TRUE(rpn_variable_del(ctxt, "tmp"));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "exists"));

    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));
    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_custom_operator() {

    rpn_context ctxt;
    
    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, "cube", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value a;
        rpn_stack_pop(ctxt, a);
        rpn_stack_push(ctxt, a*a*a);
        return 0;
    }));

    run_and_compare_ctx(ctxt, "3 cube", rpn_values(27.0));

    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_int>(2))));
    run_and_compare_ctx(ctxt, "cube", rpn_values<rpn_int>(8));

    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value(static_cast<rpn_uint>(4))));
    run_and_compare_ctx(ctxt, "cube", rpn_values(static_cast<rpn_uint>(64)));

}

void test_error_divide_by_zero() {
    run_and_error("5 0 /", rpn_value_error::DivideByZero);
    run_and_error("0 0 /", rpn_value_error::DivideByZero);
    run_and_error("105 0 mod", rpn_value_error::DivideByZero);

    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    const char* ops[] {"/", "mod"};

    for (auto& op : ops) {
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_int>(12345)}));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_int>(0)}));
        run_and_error_ctx(ctxt, op, rpn_value_error::DivideByZero);
    }
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));

    for (auto& op : ops) {
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_uint>(56789u)}));
        TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_uint>(0u)}));
        run_and_error_ctx(ctxt, op, rpn_value_error::DivideByZero);
    }
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));
}

void test_error_argument_count_mismatch() {
    rpn_context ctxt;
    rpn_operator_set(ctxt, "mismatch", 5, [](rpn_context& ctxt) -> rpn_error {
        return rpn_operator_error::CannotContinue;
    });

    run_and_error_ctx(ctxt, "12345 mismatch", rpn_operator_error::ArgumentCountMismatch);
    rpn_stack_clear(ctxt);

    run_and_error_ctx(ctxt, "1 2 3 4 mismatch", rpn_operator_error::ArgumentCountMismatch);
    rpn_stack_clear(ctxt);

    run_and_error_ctx(ctxt, "1 2 3 4 5 mismatch", rpn_operator_error::CannotContinue);
    rpn_stack_clear(ctxt);
}

void test_error_unknown_token() {
    rpn_context ctxt;

    run_and_error_ctx(ctxt, "1 2 unknown_operator_name", rpn_processing_error::UnknownOperator);
    rpn_stack_clear(ctxt);

    run_and_error_ctx(ctxt, "something_else", rpn_processing_error::UnknownOperator);
    rpn_stack_clear(ctxt);

    run_and_error_ctx(ctxt, "12345.1ertyu23", rpn_processing_error::UnknownOperator);
    rpn_stack_clear(ctxt);
}

void test_strings() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    rpn_value original { String("12345") };
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", original));
    run_and_compare_ctx(ctxt, "&value &value +", rpn_values("1234512345"));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "value", value));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "12345", value.toString().c_str(),
        "Stack string value should remain intact"
    );

    TEST_ASSERT(rpn_value("Non-empty string is true").toBoolean());
    TEST_ASSERT_FALSE_MESSAGE(rpn_value("").toBoolean(), "Empty string should be false");

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_bool() {
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));

    // parsing sometimes takes shortcuts and interprets similar 'words' as true or false
    // TODO: less clever parser
    TEST_ASSERT_FALSE(rpn_process(ctxt, "trrr"));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "fllll"));
    TEST_ASSERT_EQUAL(0, rpn_stack_size(ctxt));

    // some valid expressions
    TEST_ASSERT_TRUE(rpn_process(ctxt, "true true and"));
    stack_compare(ctxt, rpn_values(true));

    TEST_ASSERT_TRUE(rpn_process(ctxt, "false true and"));
    stack_compare(ctxt, rpn_values(false));

    // we can convert every value type
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_int>(1)}));
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_uint>(12345u)}));
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{String("test_string")}));
    TEST_ASSERT_EQUAL(3, rpn_stack_size(ctxt));

    TEST_ASSERT(rpn_process(ctxt, "and"));
    TEST_ASSERT(rpn_process(ctxt, "and"));
    stack_compare(ctxt, rpn_values(true));

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_string() {
    run_and_error("\"12345 +", rpn_processing_error::UnknownToken);
    run_and_error("12345\"", rpn_processing_error::UnknownOperator);
    run_and_compare("\"12345\"", rpn_values("12345"));
    run_and_compare("\"aaaaa\" \"bbbbb\" +", rpn_values("aaaaabbbbb"));
    run_and_error("\"aaaaa\" \"bbbbb\" -", rpn_value_error::InvalidOperation);
    run_and_error("\"aaaaa\" \"bbbbb\" /", rpn_value_error::InvalidOperation);
    run_and_error("\"aaaaa\" \"bbbbb\" *", rpn_value_error::InvalidOperation);
}

void test_parse_string_escaped() {
    run_and_compare("   \"\\x61\\x62\\x63 \\\" \"", rpn_values("abc \" "));

    run_and_error("\"\\x6\\x62\"", rpn_processing_error::UnknownToken);
    run_and_error("\"\\x\\x62\"", rpn_processing_error::UnknownToken);
    run_and_error("\"\\x\\x62\"", rpn_processing_error::UnknownToken);

    {
        char buffer[] {'\n', '"', '\\', 'n', '\\', 'n', ' ', '\\', 'n', '"', ' ', '\0'};
        run_and_compare(buffer, rpn_values("\n\n \n"));
    }

    {
        char buffer[] {'"', '\\', 't', '\\', 't', '\\', 't', '"', '\0'};
        run_and_compare(buffer, rpn_values("\t\t\t"));
    }

    {
        char buffer[] {'"', '\\', 'r', '\\', 'r', '\\', 'r', '"', '\0'};
        run_and_compare(buffer, rpn_values("\r\r\r"));
    }

    {
        char buffer[] {'"', '\\', '\\', '\\', '\\', '\\', '\\', '"', '\0'};
        run_and_compare(buffer, rpn_values("\\\\\\"));
    }

    {
        char buffer[] {'"', '\\', ' ', '"', '\0'};
        run_and_error(buffer, rpn_processing_error::UnknownToken);
    }

    {
        char buffer[] {'"', '\\', '\'', '"', '\0'};
        run_and_error(buffer, rpn_processing_error::UnknownToken);
    }
}

void test_parse_null() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "var", rpn_value { 42.0 }));
    run_and_compare_ctx(ctxt, "null &var =", rpn_values(rpn_value{}));
    run_and_compare_ctx(ctxt, "$var null eq", rpn_values(true));
    run_and_compare_ctx(ctxt, "null null eq", rpn_values(true));

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_number() {
    run_and_compare("0.0001 1 10000 / eq", rpn_values(true));
    run_and_compare("0.0001e4 1 eq", rpn_values(true));
    run_and_compare("0.0001e-4 1e-8 eq", rpn_values(true));
    run_and_compare("1.e4 10000", rpn_values(10000.0, 10000.0));
    run_and_compare("1e-4 1 10000 / eq", rpn_values(true));
    run_and_compare("1e+4 100000 eq", rpn_values(false));
    run_and_compare("1e+4 10000 eq", rpn_values(true));
    run_and_compare("1e4 10000", rpn_values(10000.0, 10000.0));
    run_and_compare("1e-4 1 eq", rpn_values(false));
    run_and_compare("1e-4 0.00001 eq", rpn_values(false));
    run_and_compare("1e-4 0.0001", rpn_values(0.0001, 0.0001));
    run_and_compare(".123 123e-3 eq", rpn_values(true));
    run_and_compare("0.123 .123 eq", rpn_values(true));
    run_and_compare("-.54321 -54321e-5 eq ", rpn_values(true));
}

void test_parse_integer() {
    run_and_compare("1u 2i 3", rpn_values(
        static_cast<rpn_uint>(1),
        static_cast<rpn_int>(2),
        static_cast<rpn_float>(3.0)
    ));

    run_and_compare("1i 1u eq", rpn_values(true));
    run_and_compare("1u 1i eq", rpn_values(true));
    run_and_compare("1u 1 eq", rpn_values(true));
    run_and_compare("1 1i eq", rpn_values(true));
    run_and_compare("1.0 1i eq", rpn_values(true));
    run_and_compare("1. 1i eq", rpn_values(true));
    run_and_compare("1u 1u eq", rpn_values(true));

    run_and_compare("1u 2i 3u 4i 5u + + + +", rpn_values<rpn_uint>(15u));
    run_and_compare("1i 2u 3i 4u 5i + + + +", rpn_values<rpn_int>(15));
    run_and_compare("1i 2i 3i 4i 5i + + + +", rpn_values<rpn_int>(15));
}

void test_parse_variable() {
    run_and_error("$ $ $", rpn_processing_error::UnknownToken);
    run_and_error("$", rpn_processing_error::UnknownToken);
    run_and_error("$var", rpn_processing_error::VariableDoesNotExist);
    run_and_compare("&var", rpn_values(rpn_value{}));
}

void test_parse_multiline() {
    const char snippet[] = R"EOF(
    1
     2
      3
       4
        5
    )EOF";
    run_and_compare(snippet, rpn_values(1.0, 2.0, 3.0, 4.0, 5.0));
}

void test_nested_stack_parse() {
    // we allow nesting stacks, but we always need to pop / close / exit them before using the previous stack
    auto values = rpn_values<rpn_float, rpn_uint, rpn_uint, rpn_uint>(0.0, 1u, 2u, 3u);
    run_and_compare("[ [ [ 0 ] ] ]", values);
}

void test_nested_stack_operator() {
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));

    // we cannot exit nested stack without creating it first
    TEST_ASSERT_FALSE(rpn_process(ctxt, "1 1 2 3 ] index"));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "] ] ] ] ] ] ] ]"));
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));

    // after exiting stack, we always get it's size. even when it is 0
    TEST_ASSERT_TRUE(rpn_process(ctxt, "[ ]"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));
    TEST_ASSERT_EQUAL(0u, rpn_stack_pop(ctxt).toUint());

    // because nested stacks do not introduce any new structures,
    // existing operators should still be able to work
    TEST_ASSERT_TRUE(rpn_process(ctxt, "1 [ 1 2 3 ] index"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(2.0, value.toFloat());
}

void test_memory() {

    // Unlike the embedded environment, we would need to play valgrind
    // and count every new / malloc and ensure we have the same amount
    // of delete / free
#if RPNLIB_PIOTEST_HOST_TEST
    TEST_IGNORE_MESSAGE("running on host");
#else
    unsigned long start = ESP.getFreeHeap();

    // Note that just running rpn_clear() will not release the memory, as internal data structures
    // will keep allocated chunks for the stack, values, variables and operators
    // However, after removing ctxt object we expect everything to be freed with it
    {
        rpn_context ctxt;
        TEST_ASSERT_TRUE(rpn_init(ctxt));
        TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", rpn_value { 5.0 }));
        TEST_ASSERT_TRUE(rpn_process(ctxt, "$value dup 1 - dup 1 - dup 1 - dup 1 -"));
        TEST_ASSERT_TRUE(rpn_clear(ctxt));
    }

    TEST_ASSERT_EQUAL_INT32(start, ESP.getFreeHeap());
#endif

}

void test_overflow() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    String a;
    auto size = ctxt.input_buffer.Size + 1;
    while (size--) {
        a += 'x';
    }

    static auto callback = [](rpn_context&) -> rpn_error {
        return 0;
    };

    String b(a.substring(1));
    String c(a.substring(2));

    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, a.c_str(), 0, callback));
    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, b.c_str(), 0, callback));
    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, c.c_str(), 0, callback));

    run_and_error_ctx(ctxt, a.c_str(), rpn_processing_error::InputBufferOverflow);
    run_and_error_ctx(ctxt, b.c_str(), rpn_processing_error::InputBufferOverflow);
    run_and_compare_ctx(ctxt, c.c_str(), rpn_values());
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int run_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_rpn_value);
    RUN_TEST(test_conversions);
    RUN_TEST(test_math);
    RUN_TEST(test_math_advanced);
    RUN_TEST(test_math_int);
    RUN_TEST(test_math_uint);
    RUN_TEST(test_trig);
    RUN_TEST(test_cast);
    RUN_TEST(test_cmp);
    RUN_TEST(test_index);
    RUN_TEST(test_map);
    RUN_TEST(test_constrain);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_stack);
    RUN_TEST(test_logic);
    RUN_TEST(test_boolean);
    RUN_TEST(test_variable);
    RUN_TEST(test_variable_operator);
    RUN_TEST(test_variable_cleanup);
    RUN_TEST(test_custom_operator);
    RUN_TEST(test_error_divide_by_zero);
    RUN_TEST(test_error_argument_count_mismatch);
    RUN_TEST(test_error_unknown_token);
    RUN_TEST(test_memory);
    RUN_TEST(test_strings);
    RUN_TEST(test_parse_string);
    RUN_TEST(test_parse_string_escaped);
    RUN_TEST(test_parse_bool);
    RUN_TEST(test_parse_null);
    RUN_TEST(test_parse_number);
    RUN_TEST(test_parse_integer);
    RUN_TEST(test_parse_variable);
    RUN_TEST(test_parse_multiline);
    RUN_TEST(test_nested_stack_parse);
    RUN_TEST(test_nested_stack_operator);
    RUN_TEST(test_overflow);
    return UNITY_END();
}

#if RPNLIB_PIOTEST_HOST_TEST

int main(int argc, char** argv) {
    return run_tests();
}

#else

void loop() {
    delay(1)
}

void setup() {
    run_tests();
}

#endif
