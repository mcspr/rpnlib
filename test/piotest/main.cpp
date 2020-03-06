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
#include <rpnlib.h>
#include <unity.h>

#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

// -----------------------------------------------------------------------------
// Helper methods
// -----------------------------------------------------------------------------

void highlight_stack_index(rpn_context& ctxt, unsigned char match, double value, double match_value) {
    char buffer[256] = {0};

    snprintf(buffer, sizeof(buffer) - 1, "Stack size = %zu", ctxt.stack.size());
    TEST_MESSAGE(buffer);

    snprintf(buffer, sizeof(buffer) - 1,
        "Expected stack[%u] == %.*ef, but it is %.*ef instead (fabs diff is %.*e, %s)",
        match,
        3, match_value,
        3, value,
        3, std::fabs(match_value - value),
        (std::fabs(match_value - value) < std::numeric_limits<double>::epsilon()) ? "less than epsilon" : "more than epsilon"
    );
    TEST_MESSAGE(buffer);

    size_t index = ctxt.stack.size() - 1;
    for (auto& stack_value : ctxt.stack) {
        char highlight = (index == match) ? '*' : ' ';
        switch (stack_value.value->type) {
            case rpn_value::i32:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: %d", highlight, index, stack_value.value->as_i32);
                break;
            case rpn_value::u32:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: %u", highlight, index, stack_value.value->as_u32);
                break;
            case rpn_value::f64:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: %f", highlight, index, stack_value.value->as_f64);
                break;
            case rpn_value::string:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: \"%s\"", highlight, index, stack_value.value->as_string.c_str());
                break;
            case rpn_value::boolean:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: %s", highlight, index, stack_value.value->as_boolean ? "true" : "false");
                break;
            case rpn_value::null:
                snprintf(buffer, sizeof(buffer) - 1, "%c %02zu: null", highlight, index);
                break;
        }
        --index;
        TEST_MESSAGE(buffer);
    }
}

void run_and_compare(const char * command, std::vector<double> expected) {

    double value;
    rpn_context ctxt;

    TEST_ASSERT_TRUE_MESSAGE(rpn_init(ctxt), "rpn_init() should return true");
    TEST_ASSERT_TRUE_MESSAGE(rpn_process(ctxt, command), "rpn_process() should return true");
    TEST_ASSERT_EQUAL_INT8_MESSAGE(RPN_ERROR_OK, rpn_error, "There should be no rpn_error set");

    TEST_ASSERT_EQUAL_MESSAGE(expected.size(), rpn_stack_size(ctxt), "Stack size does not match the expected value");
    for (unsigned char i=0; i<expected.size(); i++) {
        if (!rpn_stack_get(ctxt, i, value)) {
            highlight_stack_index(ctxt, i, value, expected[i]);
            TEST_MESSAGE(command);
            TEST_FAIL_MESSAGE("Can't get stack value at specified index");
        }
        if (std::fabs(expected[i] - value) > std::numeric_limits<double>::epsilon()) {
            highlight_stack_index(ctxt, i, value, expected[i]);
            TEST_MESSAGE(command);
            TEST_FAIL_MESSAGE("Stack value does not match the expected value");
        }
    }

}

void run_and_error(const char * command, unsigned char error_code) {

    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, command));
    TEST_ASSERT_EQUAL_INT8(error_code, rpn_error);

}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

void test_math(void) {
    run_and_compare("5 2 * 3 + 5 mod", {3.0L});
}

void test_math_advanced(void) {
#ifdef RPNLIB_ADVANCED_MATH
    run_and_compare("10 2 pow sqrt log10", {1.0L});
#else
    TEST_IGNORE_MESSAGE("fmath is disabled");
#endif
}

void test_trig(void) {
#ifdef RPNLIB_ADVANCED_MATH
    run_and_compare("pi 4 / cos 2 sqrt *", {1.0L});
#else
    TEST_IGNORE_MESSAGE("fmath is disabled");
#endif
}

void test_cast(void) {
    run_and_compare("pi 2 round pi 4 round 1.1 floor 1.1 ceil", {2.0L, 1.0L, 3.1416L, 3.14L});
}

void test_map(void) {
    run_and_compare("256 0 1024 0 100 map", {25.0L});
    run_and_compare("1 0 100 0 1000 map", {10.0L});
}

void test_index(void) {
    run_and_compare("2 10 20 30 40 50 5 index", {30.0L});
}

void test_cmp3_below(void) {
    run_and_compare("13 18 24 cmp3", {-1.0L});
}

void test_cmp3_between(void) {
    run_and_compare("18 18 24 cmp3", {0.0L});
}

void test_cmp3_above(void) {
    run_and_compare("25 18 24 cmp3", {1.0L});
}

void test_conditional(void) {
    run_and_compare("1 2 3 ifn", {2.0L});
}

void test_stack(void) {
    run_and_compare("1 3 dup unrot swap - *", {6.0L});
}

void test_logic(void) {
    run_and_compare("1 1 eq 1 1 ne 2 1 gt 2 1 lt", {0.0L, 1.0L, 0.0L, 1.0L});
}

void test_boolean(void) {
    run_and_compare("2 0 and 2 0 or 2 0 xor 1 not", {0.0L, 1.0L, 1.0L, 0.0L});
}

void test_variable(void) {

    double value;
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "tmp", double(25)));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "$tmp 5 /"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(5, value);
    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));

}

void test_variable_operator(void) {

    double value = 0.0L;
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "25 $tmp ="));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(25, value);
    TEST_ASSERT_EQUAL(1, rpn_variables_size(ctxt));

    value = 0.0L;
    TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "tmp", value));
    TEST_ASSERT_EQUAL_FLOAT(25, value);
    TEST_ASSERT_TRUE(rpn_variables_clear(ctxt));
    TEST_ASSERT_EQUAL(0, rpn_variables_size(ctxt));
}

void test_custom_operator(void) {

    double value;
    rpn_context ctxt;
    
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_operator_set(ctxt, "cube", 1, [](rpn_context & ctxt) {
        double a;
        rpn_stack_pop(ctxt, a);
        rpn_stack_push(ctxt, a*a*a);
        return true;
    }));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "3 cube"));
    TEST_ASSERT_EQUAL(1, rpn_stack_size(ctxt));
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, value));
    TEST_ASSERT_EQUAL_FLOAT(27, value);

}

void test_error_divide_by_zero(void) {
    //run_and_error("5 0 /", RPN_ERROR_DIVIDE_BY_ZERO);
    TEST_IGNORE_MESSAGE("division by zero, inf and nan are handled internally, operator will not set anything");
}

void test_error_argument_count_mismatch(void) {
    run_and_error("1 +", RPN_ERROR_ARGUMENT_COUNT_MISMATCH);
}

void test_error_unknown_token(void) {
    run_and_error("1 2 sum", RPN_ERROR_UNKNOWN_TOKEN);
}

void test_strings(void) {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", "12345"));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "$value $value +"));

    String result;
    TEST_ASSERT_TRUE(rpn_variable_get(ctxt, "value", result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "12345", result.c_str(),
        "Stack string value should remain intact"
    );

    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "1234512345", result.c_str(),
        "Stack string value should contain concatenated string"
    );

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_bool(void) {
    rpn_context ctxt;

    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_TRUE(rpn_process(ctxt, "true true and"));

    bool result;
    TEST_ASSERT_TRUE_MESSAGE(
        rpn_stack_pop(ctxt, result),
        "Stack should contain `true` value"
    );
    TEST_ASSERT_TRUE(result);

    TEST_ASSERT_TRUE(rpn_process(ctxt, "false true and"));
    TEST_ASSERT_TRUE_MESSAGE(
        rpn_stack_pop(ctxt, result),
        "Stack should contain `false` value"
    );
    TEST_ASSERT_FALSE(result);

    TEST_ASSERT_EQUAL(0, rpn_stack_size(ctxt));
    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

void test_parse_string(void) {
    rpn_context ctxt;
    TEST_ASSERT_TRUE(rpn_init(ctxt));
    TEST_ASSERT_FALSE(rpn_process(ctxt, "\"12345 +"));

    String result;
    TEST_ASSERT_FALSE_MESSAGE(
        rpn_stack_pop(ctxt, result),
        "Parser should fail without closing quote"
    );

    TEST_ASSERT_TRUE(rpn_process(ctxt, "\"12345\""));
    TEST_ASSERT_TRUE(rpn_stack_pop(ctxt, result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "12345", result.c_str(),
        "Parser should put string value on the stack"
    );

    TEST_ASSERT_FALSE(rpn_process(ctxt, "12345\""));
    TEST_ASSERT_FALSE_MESSAGE(
        rpn_stack_pop(ctxt, result),
        "Parser should fail without opening quote"
    );

    TEST_ASSERT_TRUE(rpn_clear(ctxt));
}

#if not HOST_MOCK
void test_memory(void) {

    unsigned long start = ESP.getFreeHeap();

    {
        rpn_context ctxt;
        TEST_ASSERT_TRUE(rpn_init(ctxt));
        TEST_ASSERT_TRUE(rpn_variable_set(ctxt, "value", double(5)));
        TEST_ASSERT_TRUE(rpn_process(ctxt, "$value dup 1 - dup 1 - dup 1 - dup 1 -"));
        TEST_ASSERT_TRUE(rpn_clear(ctxt));
    }

    TEST_ASSERT_EQUAL_INT32(start, ESP.getFreeHeap());

}
#else
void test_memory(void) {
    TEST_IGNORE_MESSAGE("running on host");
}
#endif

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

void setup() {
    //delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_math);
    RUN_TEST(test_math_advanced);
    RUN_TEST(test_trig);
    RUN_TEST(test_cast);
    RUN_TEST(test_map);
    RUN_TEST(test_index);
    RUN_TEST(test_cmp3_below);
    RUN_TEST(test_cmp3_between);
    RUN_TEST(test_cmp3_above);
    RUN_TEST(test_conditional);
    RUN_TEST(test_stack);
    RUN_TEST(test_logic);
    RUN_TEST(test_boolean);
    RUN_TEST(test_variable);
    RUN_TEST(test_variable_operator);
    RUN_TEST(test_custom_operator);
    RUN_TEST(test_error_divide_by_zero);
    RUN_TEST(test_error_argument_count_mismatch);
    RUN_TEST(test_error_unknown_token);
    RUN_TEST(test_memory);
    RUN_TEST(test_strings);
    RUN_TEST(test_parse_string);
    RUN_TEST(test_parse_bool);
    UNITY_END();
}

void loop() {
    delay(1);
}
