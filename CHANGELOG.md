# RPNlib change log

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.4.0] IN DEVELOPMENT
### Added
- `p` operator to print the top of the stack via debug function
- `=` operator for variable assignment in expression
- `exists` operator to check for variable existance
- allow to use either float or double as floating type, parse numbers in expressions as specified type
- add boolean type, parse `true` and `false` in expressions
- add string type, parse `"string"` in expressions
- add null type, parse `null` in expressions
- add integer and unsigned integer type, used in operators
- Allow to configure underlying types from rpnlib\_config.h and -D... flags

### Changed
- Stack structure no longer holds raw `float`, but internal `rpn_value` type
- Variables in expressions are no longer required to exist
- rpn\_... setter and getter methods now support String, Integer, Unsigned, bool and double
- Improve precision of `e` and `pi`

### Fixed
- Proper value for `e` constant

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
