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
        Serial.printf("[%02u] %s\n", index--, bool(value) ? "true" : "false");
    }

    Serial.println();
}

void dump_variables(rpn_context & ctxt) {
    Serial.printf("Variables\n--------------------\n");
    rpn_variable_foreach(ctxt, [](const String& name, const rpn_value& value) {
        Serial.printf("$%s = %.2f\n", name.c_str(), rpn_float_t(value));
    });

    Serial.println();
}

struct decode_rpn_errors {

    using callback_type = void (*)(const String&);

    decode_rpn_errors(callback_type callback) :
        callback(callback)
    {}

    void operator ()(rpn_processing_error error) {
        switch (error) {
        case RPN_ERROR_OK:
            callback("No error");
            break;
        case RPN_ERROR_UNKNOWN_TOKEN:
            callback("Unknown token");
            break;
        case RPN_ERROR_ARGUMENT_COUNT_MISMATCH:
            callback("Operator argument count mismatch");
            break;
        case RPN_ERROR_DIVIDE_BY_ZERO:
            callback("Division by zero");
            break;
        case RPN_ERROR_INVALID_OPERATION:
            callback("Invalid operation");
            break;
        case RPN_ERROR_INVALID_ARGUMENT:
            callback("Invalid argument");
            break;
        case RPN_ERROR_VARIABLE_DOES_NOT_EXIST:
            callback("Variable does not exist");
            break;
        case RPN_ERROR_STOP_PROCESSING:
            callback("Processing was stopped");
            break;
        }
    }

    void operator ()(rpn_value_error error) {
        switch (error) {
        case rpn_value_error::OK:
            callback("No error");
            break;
        case rpn_value_error::InvalidOperation:
            callback("Invalid value operation");
            break;
        case rpn_value_error::TypeMismatch:
            callback("Value type mismatch");
            break;
        case rpn_value_error::DivideByZero:
            callback("Value division by zero");
            break;
        case rpn_value_error::IEEE754:
            callback("Value floating point exception");
            break;
        case rpn_value_error::IsNull:
            callback("Value is null");
            break;
        }
    }

    void operator ()(int code) {
        callback(String("Unknown error #") + String(code));
    }

    callback_type callback;

};


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
        rpn_handle_error(ctxt.error, decode_rpn_errors([](const String& decoded) {
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
