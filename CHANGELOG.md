# RPNlib change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.24.2] XXXX-XX-XX
### Changed
- Use linked list for variables, replacing vector
- Use linked list for operators, replacing vector. Ensure we don't over-reserve space when new operators are added.

### Fixed
- Make sure we copy current stack reference with the object itself
- Context error position is now relative to the start of the input string
- Assignment operator now able to handle moved values

## [0.24.1] 2020-08-10
### Changed
- Faster parsing for strings without escape sequences
- Rework internal token buffering

## [0.24.0] 2020-08-08
### Added
- Support escaped `\n`, `\t`, `\r`, `\xFF` in strings used in expressions
- `position` member of the context's `error` object, which will be set by rpn\_process()
- For floats in expressions, either integral or fractional part can be omitted when equal to 0. But, not both at the same time.
- Add `i` and `u` suffix for integral numbers in expressions, changing default type from Float to Integer or Unsigned respectively
- Add `rpn_value::checkedToInt()`, `rpn_value::checkedToFloat()` and `rpn_value::checkedToUint()` that return error when conversion fails
- Add `inf` and `nan` operators

### Changed
- Preserve variable reference after `ifn`
- Allow negative offsets for `index`
- rpnlib\_util.h is included automatically
- `eq` will try to compare floating point numbers by comparing values distance with type's epsilon

### Fixed
- Parser no longer uses fuzzy matching for built-ins, avoiding conflict with operators
- Parser no longer uses fuzzy matching for numbers written in scientific notation
- Parser accepts more characters as end of the token, in addition to the space (` `):
  newline (`\n`), carriage return (`\r`), horizontal tab (`\t`), vertical tab (`\v`)
- Fix incorrect casts between int / uint / float when calculating numeric limits

## [0.23.0] 2020-07-26
### Added
- `p` operator to print the top of the stack via debug function
- `&var` syntax to create variable reference in expression
- `=` operator for variable assignment in expression
- `exists` operator to check for variable existance (only for references)
- `deref` operator to convert variable reference into a value (only for references)
- Allow to use either float or double as floating type, parse numbers in expressions as specified type
- Add boolean type, parse `true` and `false` in expressions
- Add null type, parse `null` in expressions
- Add string type, parse double-quoted `"string"` in expressions
- Add integer and unsigned integer type, used in operators
- Allow to configure underlying types from rpnlib\_config.h and -D... flags
- Return `rpn_error` from operators, split error types into categories
- Create a new stack by using `[` keyword. Move stack contents into the previous stack + size by using `]`.

### Changed
- Stack structure no longer holds raw `float`, but internal `rpn_value` type
- rpn\_... setter and getter methods use `rpn_value` type
- Operator functions return `rpn_error` type, allowing to return both value and operator errors
- Variables in expressions are no longer required to exist when using `&var`
  Expression will automatically create the variable, set it to `null` and push it's reference on the stack
- It is possible to create 'reference' stack values
- Improve precision of `e` and `pi`

### Fixed
- Proper value for `e` constant
- Allow to use multiple contexts simultaniously, replace `rpn_error` and `rpn_debug_callback`
  with the current `rpn_context` members `error` and `debug_callback` respectively

## [0.3.0] 2019-05-24
### Added
- Added abs operator

### Changed
- Change compare operators to eq, ne, gt, ge, lt, le

## [0.2.0] 2019-05-24
### Added
- rpn\_process will (optionally) fail if variable does not exist

### Fix
- rpn\_variable\_set will replace value if variable already exists

## [0.1.0] 2019-05-23
### Fixed
- Clear context and memory leak fix
- 
### Added
- Added more comparison operators (cmp, cmp3, index, map, constrain)
- Added casting methods (round, ceil, floor)
- AUnit testing
- Keywords file

### Changed
- Do not compile with advanced math support by default
- Renamed 'if' to 'ifn' (numeric if)

## [0.0.2] 2018-12-24
### Added
- Remote testing using Travis and PIO Plus
- Examples and more commmands
  
### Changed
- Renamed 'rpn\_begin' to 'rpn\_init'
- Variables now start with $ sign

## [0.0.1]
Initial version
