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

#pragma once

enum class rpn_error_category {
    Unknown,
    Processing,
    Operator,
    Value
};

enum class rpn_processing_error {
    Ok,
    UnknownToken,
    VariableDoesNotExist
};

enum class rpn_operator_error {
    Ok,
    CannotContinue,
    ArgumentCountMismatch,
    InvalidType,
    InvalidArgument
};

enum class rpn_value_error {
    Ok,
    InvalidOperation,
    TypeMismatch,
    DivideByZero,
    IEEE754,
    IsNull
};

struct rpn_error {
    rpn_error();

    rpn_error(int);
    rpn_error(rpn_processing_error);
    rpn_error(rpn_operator_error);
    rpn_error(rpn_value_error);

    rpn_error& operator =(rpn_processing_error);
    rpn_error& operator =(rpn_operator_error);
    rpn_error& operator =(rpn_value_error);

    bool operator ==(const rpn_error&);

    void reset();

    rpn_error_category category;
    int code;
};

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
