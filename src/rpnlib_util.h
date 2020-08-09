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

#include "rpnlib.h"
#include <utility>

template <typename Callback>
void rpn_stack_foreach(rpn_context & ctxt, Callback callback) {
    auto& stack = ctxt.stack.get();
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        callback((*it).type, *((*it).value.get()));
    }
}

template <typename Callback>
void rpn_variables_foreach(rpn_context & ctxt, Callback callback) {
    for (auto& var : ctxt.variables) {
        callback(var.name, *(var.value.get()));
    }
}

template <typename Callback>
void rpn_operators_foreach(rpn_context & ctxt, Callback callback) {
    for (auto& op : ctxt.operators) {
        callback(op.name, op.argc, op.callback);
    }
}

template <typename Visitor>
void rpn_handle_error(const rpn_error& error, Visitor&& visitor) {
    switch (error.category) {
    case rpn_error_category::Processing:
       visitor(static_cast<rpn_processing_error>(error.code));
       break;
    case rpn_error_category::Operator:
       visitor(static_cast<rpn_operator_error>(error.code));
       break;
    case rpn_error_category::Value:
       visitor(static_cast<rpn_value_error>(error.code));
       break;
    case rpn_error_category::Unknown:
       visitor(error.code);
    }
}

template <typename Callback>
struct rpn_error_decoder {

    rpn_error_decoder(Callback&& callback) :
        callback(callback)
    {}

    void operator()(rpn_processing_error error) {
        switch (error) {
        case rpn_processing_error::Ok:
            callback("No error");
            break;
        case rpn_processing_error::UnknownToken:
            callback("Unknown token");
            break;
        case rpn_processing_error::InvalidToken:
            callback("Invalid token");
            break;
        case rpn_processing_error::VariableDoesNotExist:
            callback("Variable does not exist");
            break;
        case rpn_processing_error::UnknownOperator:
            callback("Operator does not exist");
            break;
        case rpn_processing_error::NoMoreStacks:
            callback("Already in the top stack");
            break;
        case rpn_processing_error::TokenNotHandled:
            callback("Token was not handled");
            break;
        case rpn_processing_error::InputBufferOverflow:
            callback("Token is larger than the available buffer");
            break;
        }
    }

    void operator()(rpn_operator_error error) {
        switch (error) {
        case rpn_operator_error::Ok:
            callback("No error");
            break;
        case rpn_operator_error::ArgumentCountMismatch:
            callback("Operator argument count mismatch");
            break;
        case rpn_operator_error::InvalidType:
            callback("Invalid operation type");
            break;
        case rpn_operator_error::InvalidArgument:
            callback("Invalid argument");
            break;
        case rpn_operator_error::CannotContinue:
            callback("Processing was stopped, cannot continue");
            break;
        }
    }

    void operator()(rpn_value_error error) {
        switch (error) {
        case rpn_value_error::Ok:
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
        case rpn_value_error::NotAnError:
            callback("Value is not an error");
            break;
        case rpn_value_error::IEEE754:
            callback("Value floating point exception");
            break;
        case rpn_value_error::IsNull:
            callback("Value is null");
            break;
        case rpn_value_error::OutOfRangeConversion:
            callback("Value out-of-range conversion");
            break;
        case rpn_value_error::ImpossibleConversion:
            callback("Value conversion is impossible");
            break;
        }
    }

    void operator()(int code) {
        callback(String("Unknown error #") + String(code));
    }

    Callback callback;

};

template <typename Callback>
rpn_error_decoder<Callback> rpn_decode_errors(Callback&& callback) {
    return rpn_error_decoder<Callback>(std::forward<Callback>(callback));
}
