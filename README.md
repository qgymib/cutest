# cutest
UnitTest framework for C.

## Features

1. Absolutely no memory allocation. You are safe to observe and measure your own program's memory usage.
2. Tests are automatically registered when declared. No need to rewrite your test name!
3. A rich set of assertions.
4. Value-parameterized tests.

## Quick start

### Step 1. Call entrypoint function in your `main()`

```c
int main(int argc, char* argv[]) {
    return cutest_run_tests(argc, argv, stdout, NULL);
}
```

### Step 2. Write your test code

```c
TEST(simple, test) {
    ASSERT_NE_STR("hello", "world");
}
```

### Step 3. Nothing more!

You are done for everything! Compile your code and run, you will have following output:

```
[==========] total 1 test registered.
[ RUN      ] simple.test
[       OK ] simple.test (0 ms)
[==========] 1/1 test case ran. (0 ms total)
[  PASSED  ] 1 test.
```

## Integration

### CMake

Add following code to your CMakeLists.txt:

```cmake
add_subdirectory(cutest)
target_link_libraries(${YOUR_TEST_EXECUTABLE} PRIVATE cutest)
```

Remember to replace `${YOUR_TEST_EXECUTABLE}` with your actual executable name.

### Manually

Just copy `cutest.h` (in `include/` directory) and `cutest.c` (in `src/` directory) to your build tree, and you are done.

Please do note that `cutest.c` use `#include "cutest.h"` syntax to find the header file, so be sure it can be found.

## Documents

Checkout [Online manual](https://qgymib.github.io/cutest/) for API reference.
