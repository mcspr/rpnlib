# RPNlib

[![version](https://img.shields.io/github/v/tag/mcspr/rpnlib)](https://github.com/mcspr/rpnlib/blob/master/CHANGELOG.md)
[![CI](https://github.com/mcspr/rpnlib/workflows/PlatformIO%20CI/badge.svg?branch=master)](https://github.com/mcspr/rpnlib/actions?query=workflow%3A%22PlatformIO+CI%22)
[![license](https://img.shields.io/github/license/mcspr/rpnlib)](LICENSE)

**Notice** this is the fork of the original [rpnlib](https://github.com/xoseperez/rpnlib) by **[@xoseperez](https://github.com/xoseperez)**

Main differences are:
- Configurable floating point type, can replace `float` (old value type) with `double` (new default)
- String support in expressions and variables
- Variable manipulation in expressions

## Description

RPNlib is a **Reverse Polish Notation** calculator for ESP8266 & ESP32 microcontrollers.  
The library accepts a c-string with commands to execute and provides methods to evaluate the output.
It is meant to be embedded into third party software as a way to provide the user with a simple way of implementing a scripting language.

## RPN

First, you should familiarize yourself with RPN calculation.  
[Reverse Polish notation (RPN)](https://en.wikipedia.org/wiki/Reverse_Polish_notation), also known as Polish postfix notation or simply postfix notation, is a mathematical notation in which operators follow their operands, in contrast to Polish notation (PN), in which operators precede their operands. It does not need any parentheses as long as each operator has a fixed number of operands. The description "Polish" refers to the nationality of logician Jan Łukasiewicz, who invented Polish notation in 1924.

A simple calculation in infix notation might look like this:

```
( 4 - 2 ) * 5 + 1 =
```

The same calculation in RPN (postfix) will look like this:

```
4 2 - 5 * 1 +
```

It results in a shorter expression since parenthesis are unnecessary. Also the equals sign is not needed since all results are stored in the stack. From the computer point of view is much simpler to evaluate since it doesn't have to look forward for the operands.

## Library usage

The RPNlib is not an object-based (OOP) library but a context-based set of methods. This means you don't instantiate a library object but instead, you create a data context object that is passed along to all methods in the library.

A simple code would be:

* Create the context (where stack, variables and operators are stored)

```cpp
rpn_context ctxt;
```

* Initialize the context. Loads default operators via `rpn_init(ctxt)` or `rpn_operators_init(ctxt)`. Operator functions are not shared between contexts.
```cpp
rpn_init(ctxt);
```

* *Optional* Add any required variables.
```cpp
rpn_value variable { static_cast<rpn_int>(12345) };
rpn_variable_set(ctxt, "variable", variable);
```

* *Optional* Add a custom operator. Note that operator functions return `rpn_error` object.
```cpp
// takes 1 `<integer>` argument and pushes back `<integer> + 5`
rpn_operator_set(ctxt, "operator", 1, [](rpn_context& ctxt) -> rpn_error {
    rpn_value value;
    rpn_stack_pop(ctxt, value);

    value = rpn_value { static_cast<rpn_int>(5) + value.toInt() };
    rpn_stack_push(ctxt, value);

    return 0; // generic integer success code
});
```

* Process an expression string.
```cpp
rpn_process(ctxt, "4 2 - 5 * 1 +");
```

* *Optional* Use variable in the expression
```cpp
rpn_process(ctxt, "$variable $variable -");
```

* Inspect stack
```cpp
Serial.printf("Stack size: %zu\n", rpn_stack_size(ctxt));

size_t index = 0;
rpn_stack_foreach(ctxt, [&index](rpn_stack_value::Type, rpn_value& value) { // NOTE: direct access to the stack value object
    Serial.printf("Stack level #%u value: %f\n", index++, value.toFloat());
});
```

* *Optional* Inspect variables
```cpp
Serial.printf("Variables: %zu\n", rpn_variables_size(ctxt));

size_t index = 0;
rpn_variables_foreach(ctxt, [&index](const String& name, const rpn_value& value) {
    Serial.printf("Variable #%u, %s = %f\n", index++, name.c_str(), value.toFloat());
});
```

* Clear the context object. This removes everything on the stack, clears variables and all known operators.
```cpp
rpn_clear(ctxt);
```

## Expressions

### Parsing

- Tokens are separated by one ore more spaces (ASCII character code `32` / ` `).
- Every expression is interpretted left-to-right, until the token cannot be parsed.
- When a valid token is found and is not an operator, it is immediately placed on the current stack.
- When a valid token is an operator, function associated with it is immediately called.

### Default types

* Keyword `null` is reserved for the internal 'Null' type.
* Keywords `true` and `false` are reserved for the internal 'Boolean' type.
* Numbers in expressions are represented as `rpn_float` (configurable type, either `float` or `double`).
* Integer values are represented as `rpn_int`, can be used in operators.
* Unsigned integer values are represented as `rpn_uint`, can be used in operators.
* All strings are represented as `String` (Arduino class). Strings in expressions are surrounded by double quotation marks.

### Nested stacks

* Keyword `[` creates a new stack. Any expression after that point uses the new stack. Previous stack is kept in memory.
* Keyword `]` moves all of the current stack contents into a previous one, inserting from the top. After that, appends it's size at the top and destroys the current stack. Any expression after that point uses the previous stack.

### Variables

* Variable names are complete words, without spaces.
* Variables are accessed by using `$` (by value) or `&` (by reference) before the word.
* When variable has not been set yet, `$name` will result in an error.
* When variable has not been set yet, `&name` will push it's value reference to the stack with default value `Null`
* When variable reference is specified multiple times, stack elements refer to the same underlying value.
* When variable value is specified multiple times, stack elements contain copies of the underlying value.
* When variable reference is duplicated using built-in operators, new stack element refers to the same underlying value.
* When variable set to 'Null' is finally removed from the stack it will be removed from the heap too.

### Operators

* Operator and variable names are case-sensitive.
* Logical operations assume 'Boolean' and will cast every other type into it. Numeric values not equal to 0 are `true`, and `false` otherwise.
* All operators will throw an error if the number of available elements in the stack is less than the expected value.
* Some operators may throw an error when given argument type does not match the expected type
* Some operators may return different results depending on the type of elements.
* Some operators perform an automatic cast of the elements taken from the stack.

|Name|Stack operation|Description|
|-|-|-|
|`+`|( a b -- c ) | sum a and b (when a and b types are different, convers b type into the type of a before doing the operation) |
|`-`|( a b -- c ) | subtract b from a |
|`*`|( a b -- c ) | multiply b by a |
|`/`|( a b -- c ) | divide a by b (ends execution if b equals 0) |
|`mod`|( a b -- c ) |  remainder of the division of b by a |
|`abs`|( a -- b ) | absolute value of the a (integer or float) |
|`round`|( a n -- b ) | round a (float) to the n-th decimal |
|`ceil`|( a -- b ) |  b is the smallest integral value not less than a |
|`floor`|( a -- b ) |  b is the largest integral value not greater than a |
|`int`|( a -- b ) |  alias for "floor" |
|`eq`|( a b -- c ) | put true (boolean) on the stack when a equals to b, false (boolean) otherwise |
|`ne`|( a b -- c ) | a is not equal to b |
|`gt`|( a b -- c ) | a is greater than b |
|`ge`|( a b -- c ) | a is greater than or equal to b |
|`lt`|( a b -- c ) | a is less than b |
|`le`|( a b -- c ) | a is less than or equal to b |
|`cmp`|( a b -- c ) |  c (integer) is -1 if a<b, 0 if a==b and 1 if a>b |
|`cmp3`|( a b c -- d ) |  d (integer) is -1 if a<b, 1 if a>c and 0 if equals to b or c or in the middle|
|`index`|( a v1 v2 ... b -- c ) |  returns the a-nth value from the v# list, b is the number of values in the v# list. also accepts the result of `]` as list's length |
|`map`| ( a b c d e -- f ) |  performs a rule of 3 mapping of the value a (number) which goes from b to c to d to e |
|`constrain`| (a b c -- d) |  ensures a is between the range of b and c (inclusive) |
|`and`|( a b -- c ) | logical operation on the stack |
|`or`|( a b -- c ) | |
|`xor`|( a b -- c ) | only one of a or b can be converted into boolean true |
|`not`|( a -- b ) | |
|`dup`|( a -- a a ) | (note that references are duplicated instead of the values themselves) |
|`dup2`|( a b -- a b a b ) | |
|`swap`|( a b -- b a ) | |
|`rot`|( a b c -- b c a ) | |
|`unrot`|( a b c -- c a b ) | |
|`drop`|( a -- ) | |
|`over`|( a b -- a b a ) | |
|`depth`|( -- n ) |  where n (unsigned) is the number of elements on the stack |
|`exists`| (&var -- &var) | ends execution if var isn't an active variable |
|`deref`| (&var -- $var) | takes the value from var and copies it on the stack |
|`=`|( a &var -- &var ) |  sets var to the value of a and keeps var reference on the stack |
|`ifn`|( a b c -- b or c ) | if a (boolean) is true, keeps b on the stack. otherwise, keeps c |
|`end`|( a -- ) |  ends execution if a (boolean) is false |

Some operators are used in place of constant variables:

|Name|Stack operation|Description|
|-|-|-|
|`pi`|( -- a ) |  where a (float) is the value of PI |
|`e`|( -- a ) |  where a (float) is the value of e (base of the Napierian (Naperian) logarithm)|
|`inf`|( -- a ) |  where a (float) is the value of floating point inf |
|`nan`|( -- a ) |  where a (float) is the value of floating point quiet NaN |

In addition, when using `RPNLIB_ADVANCED_MATH` flag, these operators are available for floating point numbers:
|Name|Stack operation|Description|
|-|-|-|
|`sqrt`|  ( a -- b ) | where b is the square root of a (note: ends execution when a is not float) |
|`log`|  ( a -- b ) | where b is the log of a |
|`log10`|  ( a -- b ) | where b is the log10 of a |
|`exp`|  ( a -- b ) | where b is exponential power of a (e^a)  |
|`fmod`|  ( a b -- c ) | where c is the reminder of the a/b division |
|`pow`|  ( a b -- c ) | where c is a power b |
|`cos`|  ( a -- b ) | where b is cos(a). a is specified in radians |
|`sin`|  ( a -- b ) | where b is sin(a). a is specified in radians |
|`tan`|  ( a -- b ) | where b is tan(a). a is specified in radians |

## Maintainer's notice

To upload a new release:

```shell
$ # modify library.* to include a new version
$ git add library.*
$ git commit -m "Version $VERSION"
$ git tag -a -m $VERSION $VERSION
$ git push --follow-tags
```

## License

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
