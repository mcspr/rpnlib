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

// time_t value on ESP8266 could be either 4 or 8 bytes, but it might not always be able to fit into rpn_int
// (but, it is possible to tweak time funcs to accept time_t as LOW and HIGH parts, requiring special operators to handle such values)
static_assert(sizeof(rpn_int) >= sizeof(time_t), "time_t should be able to fit into a single rpn_value");

void dump_stack(rpn_context & ctxt) {
    rpn_value value;
    auto index = rpn_stack_size(ctxt) - 1;
    Serial.printf("Stack\n--------------------\n");
    while (rpn_stack_get(ctxt, index, value)) {
        Serial.printf("[%02u] %d\n", index--, value.toInt());
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
    Serial.printf("Timestamp is %ld\n", timestamp);

    // Set global timestamp value, so we can call time() later
    struct timeval tv { timestamp, 0 };
    struct timezone tz { 0, 0 };
    if (0 != settimeofday(&tv, &tz)) {
        Serial.println("Time was not set, can't run the test!");
        return;
    }

    Serial.println("Time set to:");
    Serial.println(asctime(&current_time));
    Serial.printf("Timestamp is %ld\n", time(nullptr));

    // Create context
    rpn_context ctxt;
    
    // Initialize context
    rpn_init(ctxt);

    // Add custom time functions
    rpn_operator_set(ctxt, "now", 0, [](rpn_context & ctxt) -> rpn_error {
        rpn_value result { static_cast<rpn_int>(time(nullptr)) };
        rpn_stack_push(ctxt, result);
        return 0;
    });
    rpn_operator_set(ctxt, "dow", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value value;
        rpn_stack_pop(ctxt, value);
        time_t ts = value.toInt();

        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);

        rpn_value result { rpn_int(tm_from_ts.tm_wday) };
        rpn_stack_push(ctxt, result);

        return 0;
    });
    rpn_operator_set(ctxt, "hour", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value value;
        rpn_stack_pop(ctxt, value);
        time_t ts = value.toInt();
        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);
        rpn_value result { rpn_int(tm_from_ts.tm_hour) };
        rpn_stack_push(ctxt, result);
        return 0;
    });
    rpn_operator_set(ctxt, "minute", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value value;
        rpn_stack_pop(ctxt, value);
        time_t ts = value.toInt();
        struct tm tm_from_ts;
        localtime_r(&ts, &tm_from_ts);
        rpn_value result { rpn_int(tm_from_ts.tm_min) };
        rpn_stack_push(ctxt, result);
        return 0;
    });

    rpn_variable_set(ctxt, "time",
        rpn_value(static_cast<rpn_int>(time(nullptr)))
    );

    rpn_variables_foreach(ctxt, [](const char* name, const rpn_value& value) {
        Serial.println(name);
        Serial.println(value.toInt());
    });

    // Process command
    Serial.println("Push `day of week`, `hour` and `minute` to the stack");
    if (!rpn_process(ctxt, "$time dup dup dow rot hour rot minute ")) {
        Serial.println("...Failed!");
    }
    
    // Show final stack
    dump_stack(ctxt);

    // Clear the context and free resources
    rpn_clear(ctxt);

}

void loop() {
    delay(10);
}
