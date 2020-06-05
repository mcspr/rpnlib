# RPNlib

> **Notice** this is the fork of the original rpnlib
> 
> Main differences are:
> - double instead of float as number type
> - String support in expressions and variables
> - variable assignment in expressions

RPNlib is a **Reverse Polish Notation** calculator for ESP8266 & ESP32 microcontrollers. 
The library accepts a c-string with commands to execute and provides methods to evaluate the output.
It is meant to be embedded into third party software as a way to provide the user a simple to implement scripting language.

[![version](https://img.shields.io/badge/version-0.4.0--pre9-brightgreen.svg)](CHANGELOG.md)
[![codacy](https://img.shields.io/codacy/grade/dca10aead98240db83c23ef550b591dc/master.svg)](https://www.codacy.com/app/mcspr/rpnlib/dashboard)
[![travis](https://travis-ci.org/mcspr/rpnlib.svg?branch=master)](https://travis-ci.org/mcspr/rpnlib)
[![license](https://img.shields.io/github/license/mcspr/rpnlib.svg)](LICENSE)
<br />
<br />
**[@xoseperez](https://github.com/xoseperez)**:
<br />
[![donate](https://img.shields.io/badge/donate-PayPal-blue.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=xose%2eperez%40gmail%2ecom&lc=US&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHostedGuest)
[![twitter](https://img.shields.io/twitter/follow/xoseperez.svg?style=social)](https://twitter.com/intent/follow?screen_name=xoseperez)

## RPN

First you should familiarize yourself with RPN calculation. 
Reverse Polish notation (RPN), also known as Polish postfix notation or simply postfix notation, is a mathematical notation in which operators follow their operands, in contrast to Polish notation (PN), in which operators precede their operands. It does not need any parentheses as long as each operator has a fixed number of operands. The description "Polish" refers to the nationality of logician Jan Łukasiewicz, who invented Polish notation in 1924.

A simple calculation in infix notation might look like this:

```
( 4 - 2 ) * 5 + 1 =
```

The same calculation in RPN (postfix) will look like this:

```
4 2 - 5 * 1 +
```

It results in a shorter expression since parenthesis are unnecessary. Also the equals sign is not needed since all results are stored in the stack. From the computer point of view is much simpler to evaluate since it doesn't have to look forward for the operands.

Check this [wiki page on the topic](https://en.wikipedia.org/wiki/Reverse_Polish_notation).

## Library usage

The RPNlib is not an object-based (OOP) library but a context-based set of methods. This means you don't instantiate a library object but instead, you create a data context object that is passed along to all methods in the library.

Using the library is pretty easy. Follow this steps:

* Create the context (where stack, variables and operators are stored)
* Initialize the context (loads default operators)
* Add any required variables (optional)
* Add any additional custom operators (optional)
* Process a command
* Inspect stack
* Clear context

A simple code would be:

```
rpn_context ctxt;
rpn_init(ctxt);
rpn_process(ctxt, "4 2 - 5 * 1 +");

auto size = rpn_stack_size(ctxt);
Serial.printf("Stack size: %d\n", size);

rpn_value value;
for (unsigned char i=0; i < size; i++) {
    rpn_stack_pop(ctxt, value.toFloat());
    Serial.printf("Stack level #%u value: %f\n", i, value);
}

rpn_clear(ctxt);
```

## Supported operators

This is a list of supported operators with their stack behaviour. 

* Operators (and variables) are case-sensitive.
* All operators will throw an error if the number of available elements in the stack is less than the required parameters.
* All stack elements are stored in heap.
* All numbers in the stack are repesente `double` (fundamental type).
* All strings in the stack are repesented as `String` (Arduino class).
* Some operators may return different results depending on the type of elements.
* Some operators perform an automatic cast of the elements poped from the stack.
* A boolean cast will be false if the element is 0, true otherwise.
* True is represented as 1, whilst false is represented as 0. It is possible to directly access boolean value, without the cast.


```

pi      ( -> a ) where a is the value of PI
e       ( -> a ) where a is the value of e (base of the neperian logarithms)

+       ( a b -> a+b )
-       ( a b -> a-b )
*       ( a b -> a*b )
/       ( a b -> a/b ) throws error if b==0
mod     ( a b -> a\b ) returns the reminder for the a/b division as integers

round   ( a n -> b ) where b is a rounded to the n-th decimal
ceil    ( a -> b ) where b is a rounded to the closes greater or equal integer
floor   ( a -> b ) where b is a rounded to the closes lesser or equal integer
int     ( a -> b ) alias for "floor"

sqrt    ( a -> sqrt(a) ) *
log     ( a -> log(a) ) *
log10   ( a -> log10(a) ) *
exp     ( a -> e^a ) *
fmod    ( a b -> a\b ) returns the reminder for the a/b division as real numbers *
pow     ( a b -> a^b ) *
cos     ( a -> cos(a) ) a in radians *
sin     ( a -> sin(a) ) a in radians *
tan     ( a -> tan(a) ) a in radians *

eq      ( a b -> a==b )
ne      ( a b -> a!=b )
gt      ( a b -> a>b )
ge      ( a b -> a>=b )
lt      ( a b -> a<b )
le      ( a b -> a<=b )

cmp     ( a b -> c ) c is -1 if a<b, 0 if a==b and 1 if a>b
cmp3    ( a b c -> d ) d is -1 if a<b, 1 if a>c and 0 if equals to b or c or in the middle
index   ( a v1 v2 ... b -> c ) returns the a-nth value from the v# list, b is the number of values in the v# list
map     ( a b c d e -> f ) performs a rule of 3 mapping value a which goes from b to c to d to e
constrain   (a b c -> d) ensures a is between (and included) b and c

and     ( a b -> c ) where c is 1 if both a and b are different from 0
or      ( a b -> c ) where c is 1 if a or b are different from 0
xor     ( a b -> c ) where c is 1 if either a or b are different from 0, but not both
not     ( a -> !a ) where b is 1 if a is 0, 0 otherwise

dup     ( a -> a a )
dup2    ( a b -> a b a b )
swap    ( a b -> b a )
rot     ( a b c -> b c a )
unrot   ( a b c -> c a b )
drop    ( a ->  )
over    ( a b -> a b a )
depth   ( a b c ... -> a b c ... n ) where n is the number of elements in the stack

ifn     ( a b c -> d ) if a!=0 then b else c
end     ( a -> ...) ends execution if a resolves to false
changed ( $a -> $a ) ends execution if $a changed since the last time

=       ( a $var = -> $var ) sets $var to the value of a and keeps $var on the stack

```

Operators flagged with an asterisk (*) are only available if compiled with RPNLIB_ADVANCED_MATH build flag.

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
