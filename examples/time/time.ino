/*

RPNlib

Basic example

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
#include <time.h>
#include <sys/time.h>
#include <rpnlib.h>

void dump_stack(rpn_context & ctxt) {
    int32_t value;
    auto index = rpn_stack_size(ctxt) - 1;
    Serial.printf("Stack\n--------------------\n");
    while (rpn_stack_get(ctxt, index, value)) {
        Serial.printf("[%02u] %d\n", index--, value);
    }
    Serial.println();
}

void setup() {
    
    // Init serial communication with the computer
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println();
    
    // Initialize time
    // This would normally be set via NTP configTime() or external RTC
    // See NTP examples from Core and `man ctime`
    struct tm current_time;
    current_time.tm_hour = 17;
    current_time.tm_min = 8;
    current_time.tm_sec = 24;
    current_time.tm_mday = 26;
    current_time.tm_mon = 11;
    current_time.tm_year = 2018 - 1900;
    time_t timestamp = mktime(&current_time);
    Serial.printf("Timestamp is %d\n", timestamp);

    // Set global timestamp value, so we can call time() later
    struct timeval tv { timestamp, 0 };
    struct timezone tz { 0, 0 };
    if (0 != settimeofday(&tv, &tz)) {
        Serial.println("Time was not set, can't run the test!");
        return;
    }

    Serial.println("Time set to:");
    Serial.println(asctime(&current_time));
    Serial.printf("Timestamp is %d\n", time(nullptr));

    // Create context
    rpn_context ctxt;
    
    // Initialize context
    rpn_init(ctxt);

    // Add custom time functions
    rpn_operator_set(ctxt, "now", 0, [](rpn_context & ctxt) {
        rpn_stack_push(ctxt, (int32_t)time(nullptr));
        return true;
    });
    rpn_operator_set(ctxt, "dow", 1, [](rpn_context & ctxt) {
        time_t ts;
        rpn_stack_pop(ctxt, (int32_t&)ts);
        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);
        rpn_stack_push(ctxt, (int32_t) tm_from_ts.tm_wday);
        return true;
    });
    rpn_operator_set(ctxt, "hour", 1, [](rpn_context & ctxt) {
        time_t ts;
        rpn_stack_pop(ctxt, (int32_t&)ts);
        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);
        rpn_stack_push(ctxt, (int32_t) tm_from_ts.tm_hour);
        return true;
    });
    rpn_operator_set(ctxt, "minute", 1, [](rpn_context & ctxt) {
        time_t ts;
        rpn_stack_pop(ctxt, (int32_t&)ts);
        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);
        rpn_stack_push(ctxt, (int32_t) tm_from_ts.tm_min);
        return true;
    });


    // Process command
    Serial.println("Push `day of week`, `hour` and `minute` to the stack");
    rpn_process(ctxt, "now dup dup dow rot hour rot minute ");
    
    // Show final stack
    dump_stack(ctxt);

    // Clear the context and free resources
    rpn_clear(ctxt);

}

void loop() {
    delay(10);
}
