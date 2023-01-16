# Changelog

## v1.0.4

### Features
1. Avoid global name conflict
2. Avoid namespace affect
3. Able to compare any type using custom type system
4. Use `--test_list_types` to list support types

### BREAKING CHANGES
1. Hide unused function


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
