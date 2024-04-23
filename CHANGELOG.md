# Changelog

## v3.0.4

### Features
1. Automatic disable thread support if `Threads` not found.
2. Test case can be unregistered by `cutest_unregister_case()`.


## v3.0.3 (2024/04/23)

### Fixed
1. fix: change cmake_minimum_required to 3.5 as required.


## v3.0.2 (2024/03/27)

### Fixed
1. fix: build error when used in C++ source code files.


## v3.0.1 (2023/03/28)

### Fixed
1. fix: wrong type of arguments to formatting function


## v3.0.0 (2023/03/18)

### Features
1. Switch to linear-time string globbing algorithm

### BREAKING CHANGES
1. Rename `TEST_FIXTURE_TEAREDOWN` to `TEST_FIXTURE_TEARDOWN`
2. Remove return value of abort()


## v2.0.0 (2023/03/06)

### Features
1. Print parameter information before execute tests
2. Assertions can accept C string variable as format parameter.

### BREAKING CHANGES
1. Rename `ASSERT_TEMPLATE_EXT` to `ASSERT_TEMPLATE`


## v1.0.9 (2023/02/27)

### Fixed
1. Fix: cannot porting `cutest_porting_cvfprintf`


## v1.0.8 (2023/02/27)

### Features
1. Allow to porting specific interface

### Fixed
1. Fix: cannot porting `cutest_porting_cvfprintf`


## v1.0.7 (2023/02/27)

### Features
1. Smart print integer without `<inttypes.h>`
2. Allow user have their own `TEST_INITIALIZER`

### BREAKING CHANGES
1. Rename colorful print porting function

### Fixed
1. Fix: manual register not working


## v1.0.6 (2023/02/07)

### Features
1. Add more test cases.
2. Simplify opt parser
3. Reduce assertion stack level

### Fixed
1. Remove custom type const qualifier
2. Fix: floating number compare result is wrong


## v1.0.5 (2023/02/06)

### Fixed
1. Fix: hook is triggered when use `--help` option


## v1.0.4 (2023/02/04)

### Features
1. Avoid global name conflict
2. Avoid namespace affect
3. Custom type system support
4. Use `--test_list_types` to list support types
5. Add porting layer
6. Allow manual registeration

### BREAKING CHANGES
1. Hide unused function
2. Hide time measurement functions


## v1.0.3 (2023/01/07)

### Features
1. Avoid memory allocation when print help
2. Avoid memory allocation for pattern matching
3. Able to shuffle parameterized tests
4. Get time stamp is now thread-safe
5. Print parameterized test parameter in `--test_list_tests`
6. Allow to redirect output to file in hook

### BREAKING CHANGES
1. Remove custom log
2. Unified test hook
3. Hide colorful print functions
4. Always assert regardless whether `--test_break_on_failure` is set

### Fixed
1. Fix: pattern not working on parameterized tests
2. Remove unsafe type cast
3. Fix: parameterized test always report failed in hook


## v1.0.2 (2022/12/20)

### Features
1. Support log redirection
2. Faster shuffle algorithm

### BREAKING CHANGES
1. Remove x32 / x64 assertion methods
2. Hide some function that user should not use
3. Exit code is explicit: 0 is success, otherwise failure

### Fixed
1. Fix: crash on log if no hook passed
2. Fix: `--help` cause coredump


## v1.0.1 (2022/04/29)

### Features
1. Support custom log


## v1.0.0 (2022/03/10)

Initial release
