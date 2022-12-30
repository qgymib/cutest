# Changelog

## v1.0.3

### Features
1. Avoid memory allocation when print help
2. Avoid memory allocation for pattern matching
3. Able to shuffle parameterized tests

### Fixed
1. Fix: pattern not working on parameterized tests


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
