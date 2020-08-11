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

#include "rpnlib.h"
#include "rpnlib_stack.h"
#include "rpnlib_value.h"
#include "rpnlib_variable.h"
#include "rpnlib_operators.h"

#include <algorithm>
#include <functional>
#include <vector>

#include <cstring>
#include <cstdlib>
#include <cctype>

// ----------------------------------------------------------------------------
// Errors
// ----------------------------------------------------------------------------

rpn_error::rpn_error() :
    category(rpn_error_category::Unknown),
    code(0)
{}

rpn_error::rpn_error(int code) :
    category(rpn_error_category::Unknown),
    code(code)
{}

rpn_error::rpn_error(rpn_processing_error error) :
    category(rpn_error_category::Processing),
    code(static_cast<int>(error))
{}

rpn_error::rpn_error(rpn_operator_error error) :
    category(rpn_error_category::Operator),
    code(static_cast<int>(error))
{}

rpn_error::rpn_error(rpn_value_error error) :
    category(rpn_error_category::Value),
    code(static_cast<int>(error))
{}

rpn_error& rpn_error::operator =(rpn_processing_error error) {
    category = rpn_error_category::Processing;
    code = static_cast<int>(error);
    return *this;
}

rpn_error& rpn_error::operator =(rpn_operator_error error) {
    category = rpn_error_category::Operator;
    code = static_cast<int>(error);
    return *this;
}

rpn_error& rpn_error::operator =(rpn_value_error error) {
    category = rpn_error_category::Value;
    code = static_cast<int>(error);
    return *this;
}

bool rpn_error::operator ==(const rpn_error& other) {
    return (category == other.category) && (code == other.code);
}

void rpn_error::reset() {
    position = 0;
    category = rpn_error_category::Unknown;
    code = 0;
}

// ----------------------------------------------------------------------------
// Utils
// ----------------------------------------------------------------------------

// TODO: consider using code generation for parser?
// TODO: provide a way to define operators in expressions
//       e.g.
//       - by preserving type<->token pairs instead of passing them into the tokenizer handler
//       - by having a separate branch which fully parses everything, but instead of updating stack or calling operators preserves
//         operations order, which later will be replayed

rpn_input_buffer& rpn_input_buffer::operator+=(char c) {
    if (((_length + 1) >= Size) || _overflow) {
        _overflow = true;
        return *this;
    }

    _buffer[_length++] = c;
    _buffer[_length] = '\0';

    return *this;
}

rpn_input_buffer& rpn_input_buffer::write(const char* data, size_t data_length) {
    if (((_length + data_length + 1) > Size) || _overflow) {
        _overflow = true;
        return *this;
    }

    std::copy(data, data + data_length, _buffer + _length);
    _length += data_length;
    _buffer[_length] = '\0';

    return *this;
}

bool rpn_input_buffer::operator==(const String& other) const {
    return other.equals(_buffer);
}

void rpn_input_buffer::reset() {
    _overflow = false;
    _length = 0;
    _buffer[0] = '\0';
}

const char* rpn_input_buffer::c_str() const {
    return _buffer;
}

size_t rpn_input_buffer::length() const {
    return _length;
}

namespace {

// convert raw word to bool. we only need to match one of `true` or `false`, since we expect tokenizer to ensure we have the correct string
bool _rpn_token_as_bool(const char* token) {
    return (strcmp(token, "true") == 0);
}

// note that isspace in posix terms does not only mean literal ' ' space character. excerpt from isalpha(3):
// > These functions check whether c, which must have the value of an unsigned char or EOF, falls into a certain character class according to  the  specified  locale.
// > ...
// > isspace()
// >   checks for white-space characters.  In the "C" and "POSIX" locales, these are:
// >    space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal tab ('\t'),  and vertical tab ('\v')
// > ...
// TODO: ref. https://www.greenend.org.uk/rjk/tech/cfu.html
//       due to the toolchain setting -funsigned-char on build, we are safe from implementation detail of ctype.h
//       ofc, we could also just use a custom function here and filter char with switch statement, compiled into the exact same thing that newlib does
bool _rpn_end_of_token(char c) {
    return (c == '\0') || isspace(c);
}

bool _rpn_is_hexchar(char ch) {
    return ((ch >= '0') && (ch <= '9'))
        || ((ch >= 'a') && (ch <= 'f'))
        || ((ch >= 'A') && (ch <= 'F'));
}

uint8_t _rpn_hexchar_to_byte(char ch) {
    if ((ch >= '0') && (ch <= '9')) {
        return (ch - '0');
    } else if ((ch >= 'a') && (ch <= 'f')) {
        return 10 + (ch - 'a');
    } else if ((ch >= 'A') && (ch <= 'F')) {
        return 10 + (ch - 'A');
    } else {
        return 0;
    }
}

enum class Token {
    Unknown,
    Error,
    Null,
    Word,
    Boolean,
    Integer,
    Unsigned,
    Float,
    String,
    VariableReference,
    VariableValue,
    StackPush,
    StackPop
};

// TODO: check that callback has this signature:
// `bool(rpn_token_t, const char* ptr)`
// One option is to use std::inplace_function / function_ref:
// - https://github.com/TartanLlama/function_ref

template <typename CallbackType>
size_t _rpn_tokenize(const char* input, rpn_input_buffer& token, CallbackType callback) {
    const char *p = input;
    const char *start_of_word = nullptr;

    Token type = Token::Unknown;

// The following labels **must** only reachable by jumping into them explicitly.
// We never expect to fall-through (besides here), effectively making them a separate 'function' blocks,
//
// XXX goto warning: alternative approach would be to maintain the `buffer`, `start_of_word` and `p`
// inside of a 'fsm' context object and jump around via transition functions.

loop:

    start_of_word = p;
    type = Token::Unknown;

    if (*p == '\0') {
        goto stop_parsing;
    }

    if (isspace(*p)) {
        ++p;
        goto loop;
    }

    token.reset();

    if (*p == '&') {
        type = Token::VariableReference;
        goto on_variable;
    } else if (*p == '$') {
        type = Token::VariableValue;
        goto on_variable;
    } else if (*p == '"') {
        type = Token::String;
        goto on_string;
    } else if (isdigit(*p) || (*p == '-') || (*p == '+') || (*p == '.')) {
        type = Token::Float;
        goto on_number;
    } else if ((*p == 't') || (*p == 'f')) {
        type = Token::Boolean;
        goto on_boolean;
    } else if (*p == 'n') {
        type = Token::Null;
        goto on_null;
    } else if (*p == '[') {
        type = Token::StackPush;
        goto on_stack_change;
    } else if (*p == ']') {
        type = Token::StackPop;
        goto on_stack_change;
    } else {
        type = Token::Word;
        goto on_word;
    }

    goto loop;

// Default non-empty token
// This branch is allowed as a fallback, so implicitly switch the type

on_word:

    type = Token::Word;

    while (!_rpn_end_of_token(*p)) {
        ++p;
    }

    goto push_word;

// Some reserved words for base types
// TODO: we *could* use operator dict, but we would need un-deletable entries

on_null:

    if (*p == 'n') {
        if ((*(++p) == 'u')
            && (*(++p) == 'l')
            && (*(++p) == 'l')
            && (_rpn_end_of_token(*(++p))))
        {
            goto push_word;
        }
    }

    goto on_word;

on_boolean:

    if (*p == 't') {
        if ((*(++p) == 'r')
            && (*(++p) == 'u')
            && (*(++p) == 'e')
            && (_rpn_end_of_token(*(++p))))
        {
            goto push_word;
        }
    } else if (*p == 'f') {
        if ((*(++p) == 'a')
            && (*(++p) == 'l')
            && (*(++p) == 's')
            && (*(++p) == 'e')
            && (_rpn_end_of_token(*(++p))))
        {
            goto push_word;
        }
    }

    goto on_word;

// Stack manipulation words `[`, `]`
// TODO: same as bool and null, should these be implemented as un-deletable built-in dict?

on_stack_change:

    if (!_rpn_end_of_token(*(p + 1))) {
        goto on_word;
    }

    if (!callback(type, token)) {
        goto stop_parsing;
    }

    ++p;

    goto loop;

// Generic number matching
// - anything with a single dot, e / E digits - pass along as float
// - anything without dots could be integer or unsgined, depending on a single letter suffix
//
// TODO:
// - we don't support matching anything but the base10. allow prefix with integers?
// - since we ensure number ordering, might as well implement rpn_value assignment in-place?
// - drop the token completely when matching fails instead of passing it along as a generic word token?

on_number:

    if ((*p == '+') || (*p == '-')) {
        ++p;
        if (*p != '.' && !isdigit(*p)) {
            goto on_word;
        }
    } else if (*p == '.') {
        ++p;
        if (!isdigit(*p)) {
            goto on_word;
        }
        goto on_number_float;
    }

    while (!_rpn_end_of_token(*p)) {
        if (!isdigit(*p)) {
            switch (*p) {
            case '.':
                ++p;
                goto on_number_float;
            case 'e':
            case 'E':
                goto on_number_float;
            case 'i':
            case 'u':
                if (_rpn_end_of_token(*(p + 1))) {
                    type = (*p == 'i') ? Token::Integer :
                           (*p == 'u') ? Token::Unsigned :
                           Token::Error;
                    ++p;
                    goto push_word;
                }
                goto on_word;
            default:
                goto on_word;
            }
        }
        ++p;
    }

    goto push_word;

// we have encountered floating point dot
// only allow digits, exponent or end-of-token after this point

on_number_float:

    while (!_rpn_end_of_token(*p)) {
        if (!isdigit(*p)) {
            switch (*p) {
            case 'e':
            case 'E':
                ++p;
                if ((*p == '-') || (*p == '+')) {
                    ++p;
                }
                goto on_number_digits;
            default:
                goto on_word;
            }
        }
        ++p;
    }

    goto push_word;

// we have encountered floating point exponent
// only allow digits after this point

on_number_digits:

    while (!_rpn_end_of_token(*p)) {
        if (!isdigit(*p)) {
            goto on_word;
        }
        ++p;
    }

    goto push_word;

// $var or &var

on_variable:

    ++p;

    while (!_rpn_end_of_token(*p)) {
        ++p;
    }

    // We must have more than one character and we don't allow operators to use & and $
    if (1 == (p - start_of_word)) {
        goto push_unknown;
    }

    if ((p - start_of_word) > 1) {
        token.write(start_of_word + 1, (p - start_of_word) - 1);
    }

    goto push_token;

// "...something..."
// Unlike the above, we start buffering the token right away to allow escape sequences
//
// TODO: do we need single quote support?
// TODO: or, should those be reserved for 'char' type converting into int?

on_string:

    ++p;
    ++start_of_word;

    while (*p != '\0') {
        // we've reached the end, break right away so the action below skips this char
        if (*p == '"') {
            break;
        // support some generic escape sequences + \x61\x62\x63 hex codes
        // write what we've seen so far and continue incrementing ptr after that, as usual
        } else if (*p == '\\') {
            token.write(start_of_word, (p - start_of_word));
            switch (*(p + 1)) {
            case '"':
                token += '"';
                p += 2;
                start_of_word = p;
                break;
            case 'n':
                token += '\n';
                p += 2;
                start_of_word = p;
                break;
            case 'r':
                token += '\r';
                p += 2;
                start_of_word = p;
                break;
            case 't':
                token += '\t';
                p += 2;
                start_of_word = p;
                break;
            case '\\':
                token += '\\';
                p += 2;
                start_of_word = p;
                break;
            case 'x':
                if (_rpn_is_hexchar(*(p + 2)) && _rpn_is_hexchar(*(p + 3))) {
                    token += static_cast<char>(
                        (_rpn_hexchar_to_byte(*(p + 2)) << 4)
                        | (_rpn_hexchar_to_byte(*(p + 3)))
                    );
                    p += 4;
                    start_of_word = p;
                } else {
                    goto push_unknown;
                }
                break;
            default:
                goto push_unknown;
            }
        } else {
            ++p;
        }
    }

    if (*p == '\0') {
        goto push_unknown;
    }

    token.write(start_of_word, (p - start_of_word));
    ++p;

    goto push_token;

// We either push what we gathered so far

push_token:

    if (!callback(type, token)) {
        goto stop_parsing;
    }
    token.reset();

    goto loop;

// Or, buffer the word based on the current position

push_word:

    token.write(start_of_word, p - start_of_word);
    if (!callback(type, token)) {
        goto stop_parsing;
    }
    token.reset();

    goto loop;

// Or stop the parser completely

push_unknown:

    token.reset();
    callback(Token::Unknown, token);

stop_parsing:

    return (p - input);

}

} // namespace anonymous

// ----------------------------------------------------------------------------
// Main methods
// ----------------------------------------------------------------------------

bool rpn_process(rpn_context & ctxt, const char * input, bool variable_must_exist) {

    ctxt.error.reset();
    ctxt.input_buffer.reset();

    auto position = _rpn_tokenize(input, ctxt.input_buffer, [&](Token type, const rpn_input_buffer& token) {

        //printf(":token \"%s\" type %d\n", token.c_str(), static_cast<int>(type));
        if (!token.ok()) {
            ctxt.error = rpn_processing_error::InputBufferOverflow;
            return false;
        }

        // Is token a null, bool, number, string or a variable?
        switch (type) {

        case Token::Null:
            ctxt.stack.get().emplace_back(std::make_shared<rpn_value>());
            return true;

        case Token::Boolean:
            ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(
                _rpn_token_as_bool(token.c_str())
            ));
            return true;

        // Integer tokens contain either `i` or `u` at the end, which conversion function should ignore
        case Token::Integer: {
            char* endptr = nullptr;
            rpn_int value = strtol(token.c_str(), &endptr, 10);
            if (endptr && (endptr != token.c_str()) && endptr[0] == 'i') {
                ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(value));
                return true;
            }
            break;
        }

        case Token::Unsigned: {
            char* endptr = nullptr;
            rpn_uint value = strtoul(token.c_str(), &endptr, 10);
            if (endptr && (endptr != token.c_str()) && endptr[0] == 'u') {
                ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(value));
                return true;
            }
            break;
        }

        // Floating point does not contain any surprises, just try to parse it normally
        case Token::Float: {
            char* endptr = nullptr;
            rpn_float value = strtod(token.c_str(), &endptr);
            if (endptr && endptr != token.c_str() && endptr[0] == '\0') {
                ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(value));
                return true;
            }
            break;
        }

        case Token::String:
            ctxt.stack.get().emplace_back(std::make_shared<rpn_value>(token.c_str()));
            return true;

        case Token::VariableValue:
        case Token::VariableReference: {
            if (!token.length()) {
                ctxt.error = rpn_processing_error::UnknownToken;
                return false;
            }
            auto var = std::find_if(ctxt.variables.cbegin(), ctxt.variables.cend(), [&token](const rpn_variable& v) {
                return (token == v.name);
            });
            const bool found = (var != ctxt.variables.end());

            // either push the reference to the value or the value itself, depending on the variable token type
            if (found) {
                if (Token::VariableReference == type) {
                    ctxt.stack.get().emplace_back(rpn_stack_value::Type::Variable, (*var).value);
                } else if (Token::VariableValue == type) {
                    ctxt.stack.get().emplace_back(*((*var).value));
                }
                return true;
            // in case we want value / explicitly said to check for variable existence
            } else if ((type == Token::VariableValue) || variable_must_exist) {
                ctxt.error = rpn_processing_error::VariableDoesNotExist;
                return false;
            // since we don't have the variable yet, push uninitialized one
            } else {
                auto null = std::make_shared<rpn_value>();
                ctxt.variables.emplace_back(token.c_str(), null);
                ctxt.stack.get().emplace_back(
                    rpn_stack_value::Type::Variable, null
                );
                return true;
            }
        }

        case Token::StackPush:
            ctxt.stack.stacks_push();
            return true;

        case Token::StackPop: {
            if (ctxt.stack.stacks_size() > 1) {
                ctxt.stack.stacks_merge();
                return true;
            }

            ctxt.error = rpn_processing_error::NoMoreStacks;
            return false;
        }

        // TODO: ctxt.error.position is set down below
        case Token::Unknown:
            ctxt.error = rpn_processing_error::UnknownToken;
            return false;

        case Token::Error:
            ctxt.error = rpn_processing_error::InvalidToken;
            return false;

        // Everything else that did not go through the token matching
        // TODO: class forth systems store word inside of the stack when in compilation mode
        //       consider adding a flag that never does any stack pop / push operations and just places a 'operation' code on the stack
        //       this might bloat rpn_value though :/ (yet again)
        case Token::Word: {
            auto result = std::find_if(ctxt.operators.cbegin(), ctxt.operators.cend(), [&token](const rpn_operator& op) {
                return token == op.name;
            });

            if (result != ctxt.operators.end()) {
                if ((*result).argc > ctxt.stack.get().size()) {
                    ctxt.error = rpn_operator_error::ArgumentCountMismatch;
                    return false;
                }
                ctxt.error = ((*result).callback)(ctxt);
                return (0 == ctxt.error.code);
            }

            ctxt.error = rpn_processing_error::UnknownOperator;
            return false;
        }

        }

        // Don't know the token. And, somehow missed the above switch-case
        ctxt.error = rpn_processing_error::TokenNotHandled;
        return false;

    });

    if (0 != ctxt.error.code) {
        ctxt.error.position = position;
    }

    // clean-up temporaries when
    // - variable is only referenced from the ctxt.variables (since we enforce shared_ptr copy, avoiding weak_ptr usage)
    // - value contents is either null or an error
    rpn_variables_unref(ctxt);

    return (0 == ctxt.error.code);

}

bool rpn_debug(rpn_context & ctxt, rpn_context::debug_callback_type callback) {
    ctxt.debug_callback = callback;
    return true;
}

bool rpn_init(rpn_context & ctxt) {
    return rpn_operators_init(ctxt);
}

bool rpn_clear(rpn_context & ctxt) {
    rpn_operators_clear(ctxt);
    rpn_variables_clear(ctxt);
    rpn_stack_clear(ctxt);
    return true;
}
