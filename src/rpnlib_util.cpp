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

#include "rpnlib_util.h"

void rpn_decode_errors::operator ()(rpn_processing_error error) {
    switch (error) {
    case rpn_processing_error::Ok:
        callback("No error");
        break;
    case rpn_processing_error::UnknownToken:
        callback("Unknown token");
        break;
    case rpn_processing_error::VariableDoesNotExist:
        callback("Variable does not exist");
        break;
    }
}

void rpn_decode_errors::operator ()(rpn_operator_error error) {
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

void rpn_decode_errors::operator ()(rpn_value_error error) {
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
    case rpn_value_error::IEEE754:
        callback("Value floating point exception");
        break;
    case rpn_value_error::IsNull:
        callback("Value is null");
        break;
    }
}

void rpn_decode_errors::operator ()(int code) {
    callback(String("Unknown error #") + String(code));
}
