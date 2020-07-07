/*

RPNlib

PlatformIO Unit Tests

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

#include <Arduino.h>
#include <unity.h>

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
        return snprintf(output, output_size - 1, "%" PRId32, value.toInt());
    case rpn_value::Type::Unsigned:
        return snprintf(output, output_size - 1, "%" PRIu32, value.toUint());
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
void run_and_compare(rpn_context & ctxt, const char* command, T expected) {
    TEST_MESSAGE(command);

    TEST_ASSERT_TRUE_MESSAGE(rpn_process(ctxt, command),
            "rpn_process() should return true");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ctxt.error.code,
            "There should be no error code set");

    stack_compare(ctxt, expected);
}

template <typename T>
void stack_compare(rpn_context& ctxt, T expected) {
    TEST_ASSERT_EQUAL_MESSAGE(expected.size(), rpn_stack_size(ctxt),
            "Stack size does not match the expected value");

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
            TEST_FAIL_MESSAGE(buffer);
        }

        if ((*expect) != (*stack)) {
            auto offset = sprintf(buffer, "Index %zu VALUE MISMATCH : Expected ", index);
            offset += explain_contents(buffer + offset, sizeof(buffer) - offset, (*expect));
            offset += sprintf(buffer + offset, ", Got ");
            offset += explain_contents(buffer + offset, sizeof(buffer) - offset, (*stack));
            TEST_FAIL_MESSAGE(buffer);
        }

        --index;
        ++expect;
        ++stack;
    }
}

template <typename T>
void run_and_compare(const char* command, T expected) {
    rpn_context ctxt;
    TEST_ASSERT_TRUE_MESSAGE(rpn_init(ctxt),
            "rpn_init() should return true");
    run_and_compare(ctxt, command, expected);
}

void run_and_error(const char * command, rpn_error error) {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, command));
    TEST_ASSERT(error == ctxt.error);

}

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

// TODO: also check integer operations
// TODO: make sure we handle math processing errors

void test_math() {
    run_and_compare("-5 -2 -1 * * abs", rpn_values(10.0));
    run_and_compare("5 2 * 3 + 5 mod", rpn_values(3.0));
}

void test_math_advanced() {
#ifdef RPNLIB_ADVANCED_MATH
    run_and_compare("10 2 pow sqrt log10 floor", {
        rpn_value(static_cast<rpn_float>(1.0)
    });
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
    run_and_compare("pi 4 / cos 2 sqrt *", {
        rpn_value(static_cast<rpn_float>(1.0))
    });
#else
    TEST_IGNORE_MESSAGE("fmath is disabled");
#endif
}

void test_cast() {
    run_and_compare("pi 2 round pi 4 round 1.1 floor 1.1 ceil",
            rpn_values(3.14, 3.1416, 1.0, 2.0));
}

void test_map() {
    run_and_compare("256 0 1024 0 100 map", rpn_values(25.0));
    run_and_compare("1 0 100 0 1000 map", rpn_values(10.0));
}

void test_index() {
    run_and_compare("2 10 20 30 40 50 5 index", rpn_values(30.0));
}

void test_cmp3_below() {
    run_and_compare("13 18 24 cmp3", rpn_values(-1));
}

void test_cmp3_between() {
    run_and_compare("18 18 24 cmp3", rpn_values(0));
}

void test_cmp3_above() {
    run_and_compare("25 18 24 cmp3", rpn_values(1));
}

void test_conditionals() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "1 2 eq end \"test\""));
    TEST_ASSERT_EQUAL(0, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_process(ctxt, "1 2 3 ifn"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(2.0, value.toFloat());
}

void test_stack() {
    run_and_compare("1 3 dup unrot swap - *", rpn_values(6.0));
}

void test_logic() {
    run_and_compare("1 1 eq 1 1 ne 2 1 gt 2 1 lt",
            rpn_values(true, false, true, false));
}

void test_boolean() {
    run_and_compare("2 0 and 2 0 or 2 0 xor 1 not",
            rpn_values(false, true, true, false));
}

void test_variable() {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));

    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "tmp", rpn_value { 25.0 }));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "$tmp 5 /"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(5.0, value.toFloat());

    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));

}

void test_variable_operator() {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "25 $tmp ="));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(25.0, value.toFloat());
    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));

    value = rpn_value{};
    TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "tmp", value));
    TEST_ASSERT_EQUAL_FLOAT(25.0, value.toFloat());

    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));
}

void test_variable_cleanup() {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "12.3 $tmp ="));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(12.3, value.toFloat());
    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));

    TEST_ASSERT_TRUE(rpn_process(ctxt, "null $tmp ="));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "$tmp exists"));
    TEST_ASSERT_TRUE(rpn_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));
}

void test_custom_operator() {

    rpn_context ctxt;
    
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, "cube", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value a;
        rpn_stack_pop(ctxt, a);
        rpn_stack_push(ctxt, a*a*a);
        return 0;
    }));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "3 cube"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(27.0, value.toFloat());

}

void test_error_divide_by_zero() {
    run_and_error("5 0 /", rpn_value_error::DivideByZero);
}

void test_error_argument_count_mismatch() {
    run_and_error("1 +", rpn_operator_error::ArgumentCountMismatch);
}

void test_error_unknown_token() {
    run_and_error("1 2 sum", rpn_processing_error::UnknownToken);
}

void test_strings() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));

    rpn_value original { String("12345") };
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", original));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "$value $value +"));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "value", value));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "12345", value.toString().c_str(),
        "Stack string value should remain intact"
    );

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "1234512345", value.toString().c_str(),
        "Stack string value should contain concatenated string"
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
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_int>(0)}));
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{static_cast<rpn_uint>(12345u)}));
    TEST_ASSERT(rpn_stack_push(ctxt, rpn_value{String("test_string")}));
    TEST_ASSERT_EQUAL(3, rpn_stack_size(ctxt));

    TEST_ASSERT(rpn_process(ctxt, "and"));
    TEST_ASSERT(rpn_process(ctxt, "and"));
    stack_compare(ctxt, rpn_values(false));

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_string() {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "\"12345 +"));

    rpn_value value;
    TEST_ASSERT_FALSE_MESSAGE(
        rpn_stack_pop(ctxt, value),
        "Parser should fail without closing quote"
    );

    TEST_ASSERT_FALSE(rpn_process(ctxt, "12345\""));
    TEST_ASSERT_FALSE_MESSAGE(
        rpn_stack_pop(ctxt, value),
        "Parser should fail without opening quote"
    );

    TEST_ASSERT_TRUE(rpn_process(ctxt, "\"12345\""));
    stack_compare(ctxt, rpn_values("12345"));

    TEST_ASSERT_TRUE(rpn_process(ctxt, "\"aaaaa\" \"bbbbb\" +"));
    stack_compare(ctxt, rpn_values("aaaaabbbbb"));

    TEST_ASSERT_FALSE(rpn_process(ctxt, "\"aaaaa\" \"bbbbb\" -"));
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));

    TEST_ASSERT_FALSE(rpn_process(ctxt, "\"aaaaa\" \"bbbbb\" /"));
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));

    TEST_ASSERT_FALSE(rpn_process(ctxt, "\"aaaaa\" \"bbbbb\" *"));
    TEST_ASSERT_TRUE(rpn_stack_clear(ctxt));


    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_null() {
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "null $var ="));
    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_number() {
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "1e+4 10000 eq"));

    rpn_value value;
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_TRUE(value.isBoolean());
    TEST_ASSERT_TRUE(value.toBoolean());

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_nested_stack_parse() {
    rpn_context ctxt;

    // we allow nesting stacks, but we always need to pop / close / exit them before using the previous stack
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "[ [ [ 0 ] ] ]"));

    // each time we pop the nested stack, we add it's size to the top
    // entering stack 3 levels deep results in at least 3 new elements added to the top one + contents of the stacks
    TEST_ASSERT_EQUAL(4, rpn_stack_size(ctxt));
    TEST_ASSERT_EQUAL(rpn_stack_value::Type::Array, rpn_stack_inspect(ctxt));

    rpn_value value;

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL(3, value.toUint());

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL(2, value.toUint());

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL(1, value.toUint());

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(0.0, value.toFloat());
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

#if RPNLIB_PIOTEST_HOST_TEST
void test_memory() {
    TEST_IGNORE_MESSAGE("running on host");
}
#else
void test_memory() {

    unsigned long start = ESP.getFreeHeap();

    {
        rpn_context ctxt;
        TEST_ASSERT_TRUE(rpn_init(ctxt));
        TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", rpn_value { 5.0 }));
        TEST_ASSERT_TRUE(rpn_process(ctxt, "$value dup 1 - dup 1 - dup 1 - dup 1 -"));
        TEST_ASSERT_TRUE(rpn_clear(ctxt));
    }

    TEST_ASSERT_EQUAL_INT32(start, ESP.getFreeHeap());

}
#endif

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int run_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_rpn_value);
    RUN_TEST(test_math);
    RUN_TEST(test_math_advanced);
    RUN_TEST(test_math_int);
    RUN_TEST(test_math_uint);
    RUN_TEST(test_trig);
    RUN_TEST(test_cast);
    RUN_TEST(test_map);
    RUN_TEST(test_index);
    RUN_TEST(test_cmp3_below);
    RUN_TEST(test_cmp3_between);
    RUN_TEST(test_cmp3_above);
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
    RUN_TEST(test_parse_bool);
    RUN_TEST(test_parse_null);
    RUN_TEST(test_parse_number);
    RUN_TEST(test_nested_stack_parse);
    RUN_TEST(test_nested_stack_operator);
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
