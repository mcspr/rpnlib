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
        VariableValueName,
        VariableReferenceName,
        OperatorName,
        StackKeyword,
        Array,
        Block
    };

    rpn_stack_value() = delete;

    template <typename Value>
    rpn_stack_value(unsigned char block_, Type type_, Value&& value_) :
        block(block_),
        type(type_),
        value(std::make_shared<rpn_value>(std::forward<Value>(value_)))
    {}

    template <typename Value>
    rpn_stack_value(Type type_, Value&& value_) :
        rpn_stack_value(0u, type_, std::forward<Value>(value_))
    {}

    rpn_stack_value(unsigned char block_, Type type_, ValuePtr value_) :
        block(block_),
        type(type_),
        value(value_)
    {}

    rpn_stack_value(Type type_, ValuePtr value_) :
        rpn_stack_value(0u, type_, value_)
    {}

    explicit rpn_stack_value(ValuePtr ptr) :
        rpn_stack_value(Type::Value, ptr)
    {}

    explicit rpn_stack_value(rpn_value&& value_) :
        rpn_stack_value(Type::Value, std::move(value_))
    {}

    explicit rpn_stack_value(const rpn_value& _value) :
        rpn_stack_value(Type::Value, _value)
    {}

    unsigned char block { 0u };
    Type type { Type::Value };
    ValuePtr value;
};

struct rpn_nested_stack {
    enum class Reason {
        None,
        Main,
        Array,
        Block
    };

    using stack_type = std::vector<rpn_stack_value>;
    struct Container {
        Reason reason;
        stack_type stack;
    };

    using stacks_type = std::vector<Container>;

    rpn_nested_stack() {
        stacks_push(Reason::Main);
    }

    stack_type& get() {
        return _current->stack;
    }

    size_t size() {
        return _current->stack.size();
    }

    void pop() {
        _current->stack.pop_back();
    }

    void clear() {
        _current->stack.clear();
    }

    rpn_stack_value& back() {
        return _current->stack.back();
    }

    // notice that we clear the whole chain, not just the current stack
    void stacks_clear() {
        _stacks.resize(1);
        _current = &_stacks.back();
        _current->stack.clear();
    }

    // pop out of the stack without changing anything
    void stacks_pop() {
        if (_stacks.size() > 1) {
            _stacks.pop_back();
            _current = &_stacks.back();
        }
    }

    // create a new stack and select it as the current one
    void stacks_push(Reason reason) {
        _stacks.push_back(Container{reason, {}});
        _current = &_stacks.back();
        auto block = stacks_size();
    }

    // create a new stack and select it as the current one
    size_t stacks_size() {
        return _stacks.size();
    }

    // merge current stack with the previous one + insert size value
    // then, pop out of the stack
    bool stacks_merge(Reason reason);

    Reason stacks_reason() {
        return _current->reason;
    }

    private:

    stacks_type _stacks;
    Container* _current;
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
