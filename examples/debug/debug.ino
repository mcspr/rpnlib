/*

RPNlib

Debug example

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

void dump_stack(rpn_context & ctxt) {
    auto index = rpn_stack_size(ctxt) - 1;
    Serial.printf("Stack\n--------------------\n");

    rpn_value value;
    while (rpn_stack_get(ctxt, index, value)) {
        Serial.printf("[%02u] %s\n", index--, value.toBoolean() ? "true" : "false");
    }

    Serial.println();
}

void dump_variables(rpn_context & ctxt) {
    Serial.printf("Variables\n--------------------\n");
    rpn_variables_foreach(ctxt, [](const String& name, const rpn_value& value) {
        Serial.printf("$%s = %.2f\n", name.c_str(), value.toFloat());
    });

    Serial.println();
}

void setup() {
    
    // Init serial communication with the computer
    Serial.begin(115200);
    delay(2000);
    Serial.println();
    Serial.println();
    
    // Create context
    rpn_context ctxt;
    
    // Initialize context
    rpn_init(ctxt);

    // Define debug callback
    // The callback returns the current context and
    // the token that is going to be processed next
    rpn_debug(ctxt, [](rpn_context & ctxt, const char * token) {
        dump_stack(ctxt);
        Serial.printf("Processing: %s\n\n", token);
    });

    // Load variables
    rpn_value temperature { 22.5 };
    rpn_variable_set(ctxt, "temperature", temperature);

    rpn_variable_set(ctxt, "relay", rpn_value(true));

    // Show variables
    dump_variables(ctxt);

    // Process command
    // This command checks if the temperature is below 18 or above 21 degrees and 
    // returns the expected relay (controlling a heater) accordingly. 
    // If the temperature is in between those values the relay status does not change.
    // This is a simple hysteresis behaviour.
    // Last parameter in rpn_process forces variable check,
    // the execution will fail if the variable does not exist
    if (!rpn_process(ctxt, "$temperatue 18 21 cmp3 1 + 1 $relay 0 3 index", true)) {
        rpn_handle_error(ctxt.error, rpn_decode_errors([](const String& decoded) {
            Serial.println("rpn_process stopped after an error: ");
            Serial.println(decoded);
        }));
    }

    // Show final stack
    dump_stack(ctxt);

    // Show result
    if (rpn_stack_size(ctxt) == 1) {
        rpn_value value;
        rpn_stack_pop(ctxt, value);
        Serial.printf("Relay status should be: `%s`\n", bool(value) ? "true" : "false");
    } else {
        Serial.println("Stack should have only 1 value");
    }

    // Clear the context and free resources
    rpn_clear(ctxt);

}

void loop() {
    delay(1);
}
