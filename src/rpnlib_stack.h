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

#include "rpnlib.h"
#include "rpnlib_value.h"

#include <memory>

struct rpn_context;

// TODO: 0.5.0 direct class methods instead of c style functions
//       return this struct as 'optional' type instead of bool
struct rpn_stack_value {
    using ValuePtr = std::shared_ptr<rpn_value>;

    enum class Type {
        None,
        Value,
        Variable,
        Array
    };

    rpn_stack_value() = delete;

    template <typename Value>
    rpn_stack_value(Type type, Value&& value) :
        type(type),
        value(std::make_shared<rpn_value>(std::forward<Value>(value)))
    {}

    rpn_stack_value(Type type, ValuePtr ptr) :
        type(type),
        value(ptr)
    {}

    explicit rpn_stack_value(ValuePtr ptr) :
        rpn_stack_value(Type::Value, ptr)
    {}

    explicit rpn_stack_value(rpn_value&& value) :
        rpn_stack_value(Type::Value, std::move(value))
    {}

    explicit rpn_stack_value(const rpn_value& value) :
        rpn_stack_value(Type::Value, value)
    {}

    Type type { Type:: None };
    ValuePtr value;
};

struct rpn_nested_stack {
    using stack_type = std::vector<rpn_stack_value>;
    using stacks_type = std::vector<stack_type>;

    rpn_nested_stack() :
        _stacks({{}}),
        _current(&_stacks.back())
    {}

    rpn_nested_stack(const rpn_nested_stack& other) :
        _stacks(other._stacks),
        _current(&_stacks.back())
    {}

    rpn_nested_stack(rpn_nested_stack&& other) noexcept :
        _stacks(std::move(other._stacks)),
        _current(&_stacks.back())
    {}

    stack_type& get() {
        return *_current;
    }

    // notice that we clear the whole chain, not just the current stack
    void stacks_clear() {
        _stacks.resize(1);
        _current = &_stacks.back();
        (*_current).clear();
    }

    // pop out of the stack without changing anything
    void stacks_pop() {
        if (_stacks.size() > 1) {
            _stacks.pop_back();
            _current = &_stacks.back();
        }
    }

    // create a new stack and select it as the current one
    void stacks_push() {
        _stacks.resize(_stacks.size() + 1);
        _current = &_stacks.back();
    }

    // create a new stack and select it as the current one
    size_t stacks_size() {
        return _stacks.size();
    }

    // merge current stack with the previous one + insert size value
    // then, pop out of the stack
    void stacks_merge();

    private:

    stacks_type _stacks;
    stack_type* _current;
};

rpn_stack_value::Type rpn_stack_inspect(rpn_context & ctxt);

size_t rpn_stack_size(rpn_context &);
bool rpn_stack_clear(rpn_context &);

bool rpn_stack_push(rpn_context & ctxt, const rpn_value& value);
bool rpn_stack_push(rpn_context & ctxt, rpn_value&& value);

bool rpn_stack_get(rpn_context & ctxt, unsigned char index, rpn_value& out);
bool rpn_stack_pop(rpn_context & ctxt, rpn_value& out);

rpn_value rpn_stack_pop(rpn_context & ctxt);
rpn_value rpn_stack_get(rpn_context & ctxt, unsigned char index);
