/**
 * @mainpage CUnitTest
 * # cutest
 * UnitTest framework for C.
 *
 * ## Features
 *
 * 1. Absolutely no memory allocation. You are safe to observe and measure your own program's memory usage.
 * 2. Tests are automatically registered when declared. No need to rewrite your test name!
 * 3. A rich set of assertions. And you can register your own type.
 * 4. Value-parameterized tests.
 *
 * ## Quick start
 *
 * ### Step 1. Call entrypoint function in your `main()`
 *
 * ```c
 * int main(int argc, char* argv[]) {
 *     return cutest_run_tests(argc, argv, stdout, NULL);
 * }
 * ```
 *
 * ### Step 2. Write your test code
 *
 * ```c
 * TEST(simple, test) {
 *     ASSERT_NE_STR("hello", "world");
 * }
 * ```
 *
 * ### Step 3. Nothing more!
 *
 * You are done for everything! Compile your code and run, you will have following output:
 *
 * ```
 * [==========] total 1 test registered.
 * [ RUN      ] simple.test
 * [       OK ] simple.test (0 ms)
 * [==========] 1/1 test case ran. (0 ms total)
 * [  PASSED  ] 1 test.
 * ```
 *
 * ## Integration
 *
 * ### CMake
 *
 * Add following code to your CMakeLists.txt:
 *
 * ```cmake
 * add_subdirectory(cutest)
 * target_link_libraries(${YOUR_TEST_EXECUTABLE} PRIVATE cutest)
 * ```
 *
 * Remember to replace `${YOUR_TEST_EXECUTABLE}` with your actual executable name.
 *
 * ### Manually
 *
 * Just copy `cutest.h` (in `include/` directory) and `cutest.c` (in `src/` directory) to your build tree, and you are done.
 *
 * Please do note that `cutest.c` use `#include "cutest.h"` syntax to find the header file, so be sure it can be found.
 *
 * ## Documents
 *
 * Checkout [Online manual](https://qgymib.github.io/cutest/) for API reference.
 */
/**
 * @page FAQ
 *
 * @section C2146 Syntax error : missing 'token' before identifier ...
 *
 * It is likely that you are using version 1.x.x and trying to use syntax like
 * `ASSERT_EQ_INT(a, b, fmt, ...)` to print something custom.
 *
 * Due to a bug of MSVC ([__VA_ARGS__ incorrectly passed to nested macro as a single argument][1]),
 * we are not able to pass a `const char*` string to parameter _fmt_.
 *
 * To avoid this compiler error, use:
 * ```c
 * ASSERT_EQ_INT(a, b, "example print %d", _L);
 * ```
 *
 * instead of:
 * ```c
 * const char* fmt = "example print %d";
 * ASSERT_EQ_INT(a, b, fmt, _L);
 * ```
 *
 * [1]: (https://developercommunity.visualstudio.com/t/-va-args-incorrectly-passed-to-nested-macro-as-a-s/698476)
 */
#ifndef __C_UNIT_TEST_H__
#define __C_UNIT_TEST_H__

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Major version.
 */
#define CUTEST_VERSION_MAJOR        4

/**
 * @brief Minor version.
 */
#define CUTEST_VERSION_MINOR        0

/**
 * @brief Patch version.
 */
#define CUTEST_VERSION_PATCH        0

/**
 * @brief Development version.
 */
#define CUTEST_VERSION_PREREL       0

/**
 * @brief Ensure the api is exposed as C function.
 */
#if defined(__cplusplus)
#define TEST_C_API  extern "C"
#else
#define TEST_C_API
#endif

/**
 * @example main.c
 * A example for how to call #cutest_run_tests().
 */
/**
 * @example test_p.c
 * A example for parameterized test #TEST_P().
 */

/**
 * @defgroup TEST_DEFINE Define Test
 *
 * There are three ways to define a test:
 * + by #TEST().
 * + by #TEST_F().
 * + by #TEST_P().
 *
 * #TEST() define a simple test unit, which should be self contained.
 * 
 * ```c
 * TEST(foo, self) {\
 *     ASSERT_EQ_INT(0, 0);
 * }
 * ```
 *
 * Both #TEST_F() and #TEST_P() define a set of shared test unit, which share
 * the same setup and teardown procedure defined by #TEST_FIXTURE_SETUP() and
 * #TEST_FIXTURE_TEARDOWN().
 *
 * ```c
 * TEST_FIXTURE_SETUP(foo) {
 *     printf("setup of foo.\n");
 * }
 * TEST_FIXTURE_TEARDOWN(foo) {
 *     printf("teardown of foo.\n");
 * }
 * 
 * TEST_F(foo, normal) {
 *     ASSERT_NE_INT(0, 0);
 * }
 *
 * TEST_PARAMETERIZED_DEFINE(foo, param, int, 0, 1, 2);
 * TEST_P(foo, param) {
 *     printf("param:%d\n", TEST_GET_PARAM());
 * }
 * ```
 *
 * The #TEST_P() define a parameterized test, which require #TEST_PARAMETERIZED_DEFINE() define a set of parameterized data.
 * 
 * @{
 */

/**
 * @brief Setup test fixture
 * @param [in] fixture  The name of fixture
 * @see TEST_F
 * @see TEST_P
 */
#define TEST_FIXTURE_SETUP(fixture)    \
    static void s_cutest_fixture_setup_##fixture(void)

/**
 * @brief TearDown test suit
 * @param [in] fixture  The name of fixture
 * @see TEST_F
 * @see TEST_P
 */
#define TEST_FIXTURE_TEARDOWN(fixture)    \
    static void s_cutest_fixture_teardown_##fixture(void)

/**
 * @brief Get parameterized data
 * @snippet test_p.c GET_PARAMETERIZED_DATA
 * @see TEST_PARAMETERIZED_DEFINE
 * @see TEST_P
 * @return  The data you defined
 */
#define TEST_GET_PARAM()    \
    (_test_parameterized_data[_test_parameterized_idx])

/**
 * @brief Define parameterized data for fixture
 * @snippet test_p.c ADD_SIMPLE_PARAMETERIZED_DEFINE
 * @snippet test_p.c ADD_COMPLEX_PARAMETERIZED_DEFINE
 * @param[in] fixture   Which fixture you want to parameterized
 * @param[in] test      Which test you want to parameterized
 * @param[in] TYPE      Data type
 * @param[in] ...       Data values
 */
#define TEST_PARAMETERIZED_DEFINE(fixture, test, TYPE, ...)  \
    static void cutest_usertest_parameterized_register_##fixture##_##test(void (*cb)(TYPE*, unsigned long)) {\
        static TYPE s_parameterized_userdata[] = { __VA_ARGS__ };\
        static cutest_case_t s_tests[TEST_ARRAY_SIZE(s_parameterized_userdata)];\
        unsigned long i = 0;\
        for (i = 0; i < TEST_ARRAY_SIZE(s_tests); i++) {\
            cutest_case_init(&s_tests[i], #fixture, #test,\
                s_cutest_fixture_setup_##fixture,\
                s_cutest_fixture_teardown_##fixture,\
                (void(*)(void*, unsigned long))cb);\
            cutest_case_convert_parameterized(&s_tests[i],\
                #TYPE, TEST_STRINGIFY(__VA_ARGS__), (void*)s_parameterized_userdata, i);\
            cutest_register_case(&s_tests[i]);\
        }\
    }\
    typedef TYPE u_cutest_parameterized_type_##fixture##_##test\

/**
 * @brief Suppress unused parameter warning if #TEST_GET_PARAM() is not used.
 * @snippet test_p.c SUPPRESS_UNUSED_PARAMETERIZED_DATA
 */
#define TEST_PARAMETERIZED_SUPPRESS_UNUSED  \
    (void)_test_parameterized_data; (void)_test_parameterized_idx

/**
 * @brief Parameterized Test
 *
 * A parameterized test will run many cycles, which was defined by
 * #TEST_PARAMETERIZED_DEFINE().
 *
 * You can get the parameter by #TEST_GET_PARAM(). Each cycle the #TEST_GET_PARAM()
 * will return the matching data defined in #TEST_PARAMETERIZED_DEFINE()
 *
 * @snippet test_p.c ADD_SIMPLE_PARAMETERIZED_TEST
 *
 * @note If you declare a Parameterized Test but do not want to use #TEST_GET_PARAM(),
 *   you may get a compile time warning like `unused parameter _test_parameterized_data`.
 *   To suppress this warning, just place #TEST_PARAMETERIZED_SUPPRESS_UNUSED
 *   in the begin of your test body.
 *
 * @snippet test_p.c SUPPRESS_UNUSED_PARAMETERIZED_DATA
 *
 * @param [in] fixture  The name of fixture
 * @param [in] test     The name of test case
 * @see TEST_GET_PARAM()
 * @see TEST_PARAMETERIZED_DEFINE()
 * @see TEST_PARAMETERIZED_SUPPRESS_UNUSED
 */
#define TEST_P(fixture, test) \
    TEST_C_API void u_cutest_body_##fixture##_##test(\
        u_cutest_parameterized_type_##fixture##_##test*, unsigned long);\
    TEST_INITIALIZER(cutest_usertest_interface_##fixture##_##test) {\
        cutest_usertest_parameterized_register_##fixture##_##test(u_cutest_body_##fixture##_##test);\
    }\
    TEST_C_API void u_cutest_body_##fixture##_##test(\
        u_cutest_parameterized_type_##fixture##_##test* _test_parameterized_data,\
        unsigned long _test_parameterized_idx)

/**
 * @brief Test Fixture
 * @param [in] fixture  The name of fixture
 * @param [in] test     The name of test case
 * @see TEST_FIXTURE_SETUP
 * @see TEST_FIXTURE_TEARDOWN
 */
#define TEST_F(fixture, test) \
    TEST_C_API void cutest_usertest_body_##fixture##_##test(void);\
    static void s_cutest_proxy_##fixture##_##test(void* _test_parameterized_data,\
        unsigned long _test_parameterized_idx) {\
        TEST_PARAMETERIZED_SUPPRESS_UNUSED;\
        cutest_usertest_body_##fixture##_##test();\
    }\
    TEST_INITIALIZER(cutest_usertest_interface_##fixture##_##test) {\
        static cutest_case_t _case_##fixture##_##test;\
        cutest_case_init(&_case_##fixture##_##test, #fixture, #test,\
            s_cutest_fixture_setup_##fixture,\
            s_cutest_fixture_teardown_##fixture,\
            s_cutest_proxy_##fixture##_##test);\
        cutest_register_case(&_case_##fixture##_##test);\
    }\
    TEST_C_API void cutest_usertest_body_##fixture##_##test(void)

/**
 * @brief Simple Test
 * 
 * Define a simple test that have no setup (AKA. #TEST_FIXTURE_SETUP) and teardown
 * (AKA. #TEST_FIXTURE_TEARDOWN) stage, which should be a self contained test.
 * 
 * @snippet test.c DEFINE_SIMPLE_TEST
 * @param [in] fixture  suit name
 * @param [in] test     case name
 */
#define TEST(fixture, test)  \
    TEST_C_API void cutest_usertest_body_##fixture##_##test(void);\
    static void s_cutest_proxy_##fixture##_##test(void* _test_parameterized_data,\
        unsigned long _test_parameterized_idx) {\
        TEST_PARAMETERIZED_SUPPRESS_UNUSED;\
        cutest_usertest_body_##fixture##_##test();\
    }\
    TEST_INITIALIZER(cutest_usertest_interface_##fixture##_##test) {\
        static cutest_case_t _case_##fixture##_##test;\
        cutest_case_init(&_case_##fixture##_##test, #fixture,#test,\
            NULL, NULL, s_cutest_proxy_##fixture##_##test);\
        cutest_register_case(&_case_##fixture##_##test);\
    }\
    TEST_C_API void cutest_usertest_body_##fixture##_##test(void)

/** @cond */

/**
 * @def TEST_NARG
 * @brief Get the number of arguments
 */
#ifdef _MSC_VER // Microsoft compilers
#   define TEST_NARG(...)  \
        TEST_INTERNAL_NARG3(TEST_INTERNAL_NARG4(__VA_ARGS__))
#   define TEST_INTERNAL_NARG4(...)    \
        unused, __VA_ARGS__
#   define TEST_INTERNAL_NARG3(...)   \
        TEST_EXPAND(TEST_INTERNAL_NARG(__VA_ARGS__, \
            63, 62, 61, 60, 59, 58, 57, 56, \
            55, 54, 53, 52, 51, 50, 49, 48, \
            47, 46, 45, 44, 43, 42, 41, 40, \
            39, 38, 37, 36, 35, 34, 33, 32, \
            31, 30, 29, 28, 27, 26, 25, 24, \
            23, 22, 21, 20, 19, 18, 17, 16, \
            15, 14, 13, 12, 11, 10,  9,  8, \
             7,  6,  5,  4,  3,  2,  1,  0) \
        )
#   define TEST_INTERNAL_NARG(\
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8, \
         _9, _10, _11, _12, _13, _14, _15, _16, \
        _17, _18, _19, _20, _21, _22, _23, _24, \
        _25, _26, _27, _28, _29, _30, _31, _32, \
        _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, \
        _49, _50, _51, _52, _53, _54, _55, _56, \
        _57, _58, _59, _60, _61, _62, _63, _64, \
        count, ...) count
#else // Non-Microsoft compilers
#   define TEST_NARG(...)  \
        TEST_INTERNAL_NARG(0, ## __VA_ARGS__, 64, \
            63, 62, 61, 60, 59, 58, 57, 56, \
            55, 54, 53, 52, 51, 50, 49, 48, \
            47, 46, 45, 44, 43, 42, 41, 40, \
            39, 38, 37, 36, 35, 34, 33, 32, \
            31, 30, 29, 28, 27, 26, 25, 24, \
            23, 22, 21, 20, 19, 18, 17, 16, \
            15, 14, 13, 12, 11, 10,  9,  8, \
             7,  6,  5,  4,  3,  2,  1,  0)
#   define TEST_INTERNAL_NARG(_0, \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8, \
         _9, _10, _11, _12, _13, _14, _15, _16, \
        _17, _18, _19, _20, _21, _22, _23, _24, \
        _25, _26, _27, _28, _29, _30, _31, _32, \
        _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, \
        _49, _50, _51, _52, _53, _54, _55, _56, \
        _57, _58, _59, _60, _61, _62, _63, _64, \
        count, ...) count
#endif

 /**
  * @def TEST_BARG
  * @brief Check if any parameter exists.
  */
#if defined(_MSC_VER)
#   define TEST_BARG(...)  \
        TEST_INTERNAL_BARG3(TEST_INTERNAL_BARG4(__VA_ARGS__))
#   define TEST_INTERNAL_BARG4(...)    \
        unused, __VA_ARGS__
#   define TEST_INTERNAL_BARG3(...)   \
        TEST_EXPAND(TEST_INTERNAL_BARG(__VA_ARGS__, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 0) \
        )
#   define TEST_INTERNAL_BARG(\
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8, \
         _9, _10, _11, _12, _13, _14, _15, _16, \
        _17, _18, _19, _20, _21, _22, _23, _24, \
        _25, _26, _27, _28, _29, _30, _31, _32, \
        _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, \
        _49, _50, _51, _52, _53, _54, _55, _56, \
        _57, _58, _59, _60, _61, _62, _63, _64, \
        count, ...) count
#else
#   define TEST_BARG(...)  \
        TEST_INTERNAL_BARG(0, ## __VA_ARGS__, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 0)
#   define TEST_INTERNAL_BARG(_0, \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8, \
         _9, _10, _11, _12, _13, _14, _15, _16, \
        _17, _18, _19, _20, _21, _22, _23, _24, \
        _25, _26, _27, _28, _29, _30, _31, _32, \
        _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, \
        _49, _50, _51, _52, _53, _54, _55, _56, \
        _57, _58, _59, _60, _61, _62, _63, _64, \
        count, ...) count
#endif

#define TEST_STRINGIFY(...)     TEST_STRINGIFY_2(__VA_ARGS__)
#define TEST_STRINGIFY_2(...)   #__VA_ARGS__

#define TEST_EXPAND(x)          x
#define TEST_JOIN(a, b)         TEST_JOIN2(a, b)
#define TEST_JOIN2(a, b)        a##b

#define TEST_ARRAY_SIZE(arr)    (sizeof(arr) / sizeof(arr[0]))

/** @endcond */

/**
 * Group: TEST_DEFINE
 * @}
 */

/**
 * @defgroup TEST_MANUAL_REGISTRATION Manually register test
 *
 * @note In most case you don't need it. By default all test cases are
 *   registered automatically. If not, please check whether the value of
 *   #TEST_NEED_MANUAL_REGISTRATION is 1. If so, it means that your compiler
 *   does not support function constructor.
 *
 * @warning There is a chance that even your compiler support function
 *   constructor, the linker will remove constructor code in link stage. In
 *   such case you still need to register test manually.
 *
 * If your environment does not support automatically register test cases, you
 * need to manually register your tests.
 *
 * There are at least two ways to manually register test cases:
 *
 * ## X-Macro
 *
 * @note This works on any standard C compiler.
 *
 * Assume you have following tests defined:
 *
 * ```c
 * TEST_FIXTURE_SETUP(manual_registeration){}
 * TEST_FIXTURE_TEARDOWN(manual_registeration){}
 * TEST_PARAMETERIZED_DEFINE(manual_registeration, third, int, 0);
 *
 * TEST(manual_registeration, first){}
 * TEST_F(manual_registeration, second){}
 * TEST_P(manual_registeration, third){ TEST_PARAMETERIZED_SUPPRESS_UNUSED; }
 * ```
 *
 * To manually register your tests:
 *
 * 1. Defines all your tests in macro.
 *   ```c
 *   #define MY_TEST_TABLE(xx) \
 *       xx(manual_registeration, first) \
 *       xx(manual_registeration, second) \
 *       xx(manual_registeration, third)
 *   ```
 *
 * 2. Declare all tests interface on top of your file.
 *   ```c
 *   MY_TEST_TABLE(TEST_MANUAL_DECLARE_TEST_INTERFACE)
 *   ```
 *
 * 3. Register all tests.
 *   ```c
 *   MY_TEST_TABLE(TEST_MANUAL_REGISTER_TEST_INTERFACE)
 *   ```
 *
 * ## Section
 *
 * @note This rely on compiler and linker. Here we take gcc as example.
 *
 * First of all, you need a section name so all following magic can happen,
 * let's say `cutest`.
 *
 * 1. Custom #TEST_INITIALIZER before include `cutest.h`
 *   ```c
 *   #define TEST_INITIALIZER(f) \
 *       TEST_C_API void f(void) __attribute__((__section__(".cutest")));\
 *       TEST_C_API void f(void)
 *   
 *   #include <cutest.h>
 *   ```
 *
 * 2. Custom linker script .`lds`. Please note the code `ALIGN(0x4)`, it is the
 *    size of function pointer, which should be `0x4` on 32-bit platform and
 *    `0x8` on 64-bit platform.
 *   ```
 *   .cutest :
 *   {
 *       . = ALIGN(0x4);
 *       _cutest_entry_start = .;
 *       KEEP(*(SORT(.cutest*)));
 *       _cutest_entry_end = .;
 *       . = ALIGN(0x4);
 *   } >RAM0L
 *   ```
 *
 * 3. Load register test cases before call #cutest_run_tests()
 *   ```c
 *   extern (void(*)(void)) _cutest_entry_start;
 *   extern (void(*)(void)) _cutest_entry_end;
 *
 *   int main(int argc, char* argv[])
 *   {
 *       uintptr_t func = _cutest_entry_start;
 *       for (; func < _cutest_entry_end; func += sizeof(void*))
 *       {
 *           (void(*)(void)) f = func;
 *           f();
 *       }
 *       return cutest_run_tests(argc, argv, stdout, NULL);
 *   }
 *   ```
 *
 * @{
 */

/**
 * @def TEST_INITIALIZER(f)
 * @brief Function constructor.
 *
 * Run the following code before main() invoke.
 *
 * It generate a startup entrypoint with following function protocol that has
 * no parameter and return value:
 * ```c
 * void (*function)(void);
 * ```
 *
 * @param[f] The entrypoint name.
 */
#ifdef TEST_INITIALIZER
/*
 * Do nothing.
 * We assume user have their own way to register test entrypoint.
 *
 * @warning Function must have protocol of `void (*)(void)`.
 */
#elif __cplusplus
#   define TEST_INITIALIZER(f) \
        TEST_C_API void f(void); \
        struct f##_t_ { f##_t_(void) { f(); } }; f##_t_ f##_; \
        TEST_C_API void f(void)
#elif defined(_MSC_VER)
#   pragma section(".CRT$XCU",read)
#   define TEST_INITIALIZER2_(f,p) \
        TEST_C_API void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        TEST_C_API void f(void)
#   ifdef _WIN64
#       define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"")
#   else
#       define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"_")
#   endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__llvm__)
#   define TEST_INITIALIZER(f) \
        TEST_C_API void f(void) __attribute__((__constructor__)); \
        TEST_C_API void f(void)
#else
#   define TEST_INITIALIZER(f) \
        TEST_C_API void f(void)
#   define TEST_NEED_MANUAL_REGISTRATION    1
#endif

/**
 * @brief Whether manual registration is needed.
 *
 * If the value is 0, tests will be automatically registered. If not, you need
 * to register your tests manually.
 */
#if !defined(TEST_NEED_MANUAL_REGISTRATION)
#   define TEST_NEED_MANUAL_REGISTRATION    0
#endif

/**
 * @brief Declare test interface.
 * @param[in] fixture   Fixture name.
 * @param[in] test      Test name.
 */
#define TEST_MANUAL_DECLARE_TEST_INTERFACE(fixture, test) \
    TEST_C_API void cutest_usertest_interface_##fixture##_##test();

/**
 * @brief Call test interface.
 * @param[in] fixture   Fixture name.
 * @param[in] test      Test name.
 */
#define TEST_MANUAL_REGISTER_TEST_INTERFACE(fixture, test) \
    cutest_usertest_interface_##fixture##_##test();

/**
 * Group: TEST_MANUAL_REGISTRATION
 * @}
 */

/**
 * @defgroup TEST_DYNAMIC_REGISTRATION Dynamic register test
 *
 * Of course, you can also register your test dynamically without using any of #TEST_F(), #TEST_P() or #TEST().
 *
 * To dynamically register test, you need to register your test case by #cutest_register_case() before #cutest_run_tests(),
 * and unregister by #cutest_unregister_case() after all test cases are run.
 *
 * @warning You cannot unregister test case during #cutest_run_tests().
 *
 * @{
 */

typedef struct cutest_map_node
{
    struct cutest_map_node*     __rb_parent_color;      /**< father node | color */
    struct cutest_map_node*     rb_right;               /**< right child node */
    struct cutest_map_node*     rb_left;                /**< left child node */
} cutest_map_node_t;

/**
 * @brief Test case setup function.
 */
typedef void (*cutest_test_case_setup_fn)(void);

/**
 * @brief Test case teardown function.
 */
typedef void (*cutest_test_case_teardown_fn)(void);

/**
 * @brief Test case body function.
 * @param[in] dat - Data passed to #cutest_case_t::stage::body
 * @param[in] idx - Index passed to #cutest_case_t::stage::body
 */
typedef void (*cutest_test_case_body_fn)(void* dat, unsigned long idx);

typedef struct cutest_case
{
    cutest_map_node_t                   node;           /**< Node in rbtree. */

    struct
    {
        const char*                     fixture_name;   /**< suit name. */
        const char*                     case_name;      /**< case name. */
    } info;

    struct
    {
        cutest_test_case_setup_fn       setup;          /**< setup. */
        cutest_test_case_teardown_fn    teardown;       /**< teardown. */
        cutest_test_case_body_fn        body;           /**< test body. */
    } stage;

    struct
    {
        unsigned long                   mask;           /**< Internal mask. */
        unsigned long                   randkey;        /**< Random key. */
    } data;

    struct
    {
        const char*                     type_name;      /**< User type name. */
        const char*                     test_data_cstr; /**< The C string of user test data. */
        void*                           param_data;     /**< Data passed to #cutest_case_t::stage::body */
        unsigned long                   param_idx;      /**< Index passed to #cutest_case_t::stage::body */
    } parameterized;
} cutest_case_t;

/**
 * @brief Initialize test case as normal test.
 * @param[out] tc - Test case.
 * @param[in] fixture_name - Fixture name.
 * @param[in] case_name - Test name.
 * @param[in] setup - Setup function.
 * @param[in] teardown - Teardown function.
 * @param[in] body - Test body function.
 */
void cutest_case_init(cutest_case_t* tc, const char* fixture_name, const char* case_name,
	cutest_test_case_setup_fn setup, cutest_test_case_teardown_fn teardown, cutest_test_case_body_fn body);

/**
 * @brief Convert normal test case to parameterized test case.
 * @param[in,out] tc - Test case.
 * @param[in] type - User type name.
 * @param[in] commit - The C string of user test data.
 * @param[in] data - Data passed to #cutest_case_t::stage::body
 * @param[in] size - Index passed to #cutest_case_t::stage::body
 */
void cutest_case_convert_parameterized(cutest_case_t* tc, const char* type,
    const char* commit, void* data, unsigned long size);

/**
 * @brief Register test case.
 *
 * A registered test case will be automatically executed by #cutest_run_tests().
 *
 * @note A registered test case can not be unregistered during #cutest_run_tests().
 * @see #cutest_unregister_case().
 * @param[in,out] tc - Test case.
 */
void cutest_register_case(cutest_case_t* tc);

/**
 * @brief Unregister test case.
 * @see #cutest_register_case().
 * @param[in,out] tc - Test case.
 */
void cutest_unregister_case(cutest_case_t* tc);

/**
 * Group: TEST_DYNAMIC_REGISTRATION
 * @}
 */

/**
 * @defgroup TEST_ASSERTION Assertion
 *
 * ## Basic usage
 *
 * [cutest](https://github.com/qgymib/cutest/) support rich set of assertion. An
 * assertion typically have following syntax:
 *
 * ```c
 * ASSERT_OP_TYPE(a, b)
 * ASSERT_OP_TYPE(a, b, fmt, ...)
 * ```
 *
 * The `OP` means which compare operation you want to use:
 * + `EQ`: `a` is equal to `b`.
 * + `NE`: `a` is not equal to `b`.
 * + `LT`: `a` is less than `b`.
 * + `LE`: `a` is equal to `b` or less than `b`.
 * + `GT`: `a` is greater than `b`.
 * + `GE`: `a` is equal to `b` or greater than `b`.
 *
 * The `TYPE` means the type of value `a` and `b`.
 *
 * > To support more types, checkout #TEST_REGISTER_TYPE_ONCE().
 *
 * So, an assertion like #ASSERT_EQ_INT() means except `a` and `b` have type of
 * `int` and they are the same value.
 *
 * ## Extra custom information
 *
 * You may notice all assertions have syntax of `ASSERT_OP_TYPE(a, b, fmt, ...)`,
 * it means custom print is available if assertion fails. For example, the
 * following code
 *
 * ```c
 * int errcode = ENOENT;
 * ASSERT_EQ_INT(0, errcode, "%s(%d)", strerror(errcode), errcode);
 * ```
 *
 * Will print something like:
 *
 * ```
 * No such file or directory(2)
 * ```
 *
 * You may also want to refer to the actual value of operator, you can use `_L`
 * to refer to left operator and `_R` to refer to right operator:
 *
 * ```c
 * ASSERT_EQ_INT(0, 1 + 2, "%d is not %d", _L, _R);
 * ```
 *
 * The output will be something like:
 *
 * ```
 * 0 is not 3
 * ```
 *
 * @{
 */

/**
 * @defgroup TEST_ASSERTION_C89 C89 Assertion
 *
 * Assertion macros for C89 standard, provide native type support:
 * + char
 * + unsigned char
 * + signed char
 * + short
 * + unsigned short
 * + int
 * + unsigned int
 * + long
 * + unsigned long
 * + float
 * + double
 * + const void*
 * + const char*
 *
 * @note About `size_t` and `ptrdiff_t`: Although they are included in C89
 *   standard, the formal print conversion specifier `%%zu` and `%%td` are
 *   included in C99 standard. Besides they need `<stddef.h>`, and we want
 *   the dependency to be minimum.
 *
 * @note About `long double`:  Although it exists in C89 standard, we do not
 * offer support for type `long double`, because actual properties unspecified.
 * The implementation can be either x86 extended-precision floating-point format
 * (80 bits, but typically 96 bits or 128 bits in memory with padding bytes),
 * the non-IEEE "double-double" (128 bits), IEEE 754 quadruple-precision
 * floating-point format (128 bits), or the same as double.
 *
 * @see http://port70.net/~nsz/c/c89/c89-draft.html
 *
 * @{
 */

/**
 * @defgroup TEST_ASSERTION_C89_CHAR char
 * @{
 */
#define ASSERT_EQ_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_CHAR(a, b, ...)       ASSERT_TEMPLATE(char, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_DCHAR signed char
 * @{
 */
#define ASSERT_EQ_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_DCHAR(a, b, ...)      ASSERT_TEMPLATE(signed char, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_UCHAR unsigned char
 * @{
 */
#define ASSERT_EQ_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UCHAR(a, b, ...)      ASSERT_TEMPLATE(unsigned char, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_SHORT short
 * @{
 */
#define ASSERT_EQ_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_SHORT(a, b, ...)      ASSERT_TEMPLATE(short, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_USHORT unsigned short
 * @{
 */
#define ASSERT_EQ_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_USHORT(a, b, ...)     ASSERT_TEMPLATE(unsigned short, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_INT int
 * @{
 */
#define ASSERT_EQ_INT(a, b, ...)        ASSERT_TEMPLATE(int, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INT(a, b, ...)        ASSERT_TEMPLATE(int, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INT(a, b, ...)        ASSERT_TEMPLATE(int, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INT(a, b, ...)        ASSERT_TEMPLATE(int, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INT(a, b, ...)        ASSERT_TEMPLATE(int, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INT(a, b, ...)        ASSERT_TEMPLATE(int, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_UINT unsigned int
 * @{
 */
#define ASSERT_EQ_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINT(a, b, ...)       ASSERT_TEMPLATE(unsigned int, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_LONG long
 * @{
 */
#define ASSERT_EQ_LONG(a, b, ...)       ASSERT_TEMPLATE(long, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_LONG(a, b, ...)       ASSERT_TEMPLATE(long, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_LONG(a, b, ...)       ASSERT_TEMPLATE(long, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_LONG(a, b, ...)       ASSERT_TEMPLATE(long, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_LONG(a, b, ...)       ASSERT_TEMPLATE(long, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_LONG(a, b, ...)       ASSERT_TEMPLATE(long, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_ULONG unsigned long
 * @{
 */
#define ASSERT_EQ_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_ULONG(a, b, ...)      ASSERT_TEMPLATE(unsigned long, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_FLOAT float
 * @{
 */
#define ASSERT_EQ_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_FLOAT(a, b, ...)      ASSERT_TEMPLATE(float, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_DOUBLE double
 * @{
 */
#define ASSERT_EQ_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE(double, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_PTR const void*
 * @{
 */
#define ASSERT_EQ_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_PTR(a, b, ...)        ASSERT_TEMPLATE(const void*, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C89_STR const char*
 * @{
 */
#define ASSERT_EQ_STR(a, b, ...)        ASSERT_TEMPLATE(const char*, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_STR(a, b, ...)        ASSERT_TEMPLATE(const char*, !=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99 C99 Assertion
 *
 * Assertion macros for C99 standard, provide support:
 * + long long
 * + unsigned long long
 * + int8_t
 * + uint8_t
 * + int16_t
 * + uint16_t
 * + int32_t
 * + uint32_t
 * + int64_t
 * + uint64_t
 * + size_t
 * + ptrdiff_t
 * + intptr_t
 * + uintptr_t
 *
 * @note To use these assertions, you need `<stdint.h>` and `<inttypes.h>` header
 *   files which are not included.
 *
 * @note These assertions are enabled by default. To disable all of them, add
 * `CUTEST_NO_C99_SUPPORT` (eg. `-DCUTEST_NO_C99_SUPPORT`) during compile cutest.
 *
 * @note These assertions can be disabled separatily:
 * | type               | flag                        |
 * | ------------------ | --------------------------- |
 * | long long          | CUTEST_NO_LONGLONG_SUPPORT  |
 * | unsigned long long | CUTEST_NO_ULONGLONG_SUPPORT |
 * | int8_t             | CUTEST_NO_INT8_SUPPORT      |
 * | uint8_t            | CUTEST_NO_UINT8_SUPPORT     |
 * | int16_t            | CUTEST_NO_INT16_SUPPORT     |
 * | uint16_t           | CUTEST_NO_UINT16_SUPPORT    |
 * | int32_t            | CUTEST_NO_INT32_SUPPORT     |
 * | uint32_t           | CUTEST_NO_UINT32_SUPPORT    |
 * | int64_t            | CUTEST_NO_INT64_SUPPORT     |
 * | uint64_t           | CUTEST_NO_UINT64_SUPPORT    |
 * | size_t             | CUTEST_NO_SIZE_SUPPORT      |
 * | ptrdiff_t          | CUTEST_NO_PTRDIFF_SUPPORT   |
 * | intptr_t           | CUTEST_NO_INTPTR_SUPPORT    |
 * | uintptr_t          | CUTEST_NO_UINTPTR_SUPPORT   |
 * 
 * @{
 */

/**
 * @defgroup TEST_ASSERTION_C99_LONGLONG long long
 * @{
 */
#define ASSERT_EQ_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_LONGLONG(a, b, ...)   ASSERT_TEMPLATE(long long, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_ULONGLONG unsigned long long
 * @{
 */
#define ASSERT_EQ_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_ULONGLONG(a, b, ...)  ASSERT_TEMPLATE(unsigned long long, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_INT8 int8_t
 * @{
 */
#define ASSERT_EQ_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INT8(a, b, ...)       ASSERT_TEMPLATE(int8_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_UINT8 uint8_t
 * @{
 */
#define ASSERT_EQ_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINT8(a, b, ...)      ASSERT_TEMPLATE(uint8_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_INT16 int16_t
 * @{
 */
#define ASSERT_EQ_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INT16(a, b, ...)      ASSERT_TEMPLATE(int16_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */


/**
 * @defgroup TEST_ASSERTION_C99_UINT16 uint16_t
 * @{
 */
#define ASSERT_EQ_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINT16(a, b, ...)     ASSERT_TEMPLATE(uint16_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_INT32 int32_t
 * @{
 */
#define ASSERT_EQ_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INT32(a, b, ...)      ASSERT_TEMPLATE(int32_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_UINT32 uint32_t
 * @{
 */
#define ASSERT_EQ_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINT32(a, b, ...)     ASSERT_TEMPLATE(uint32_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_INT64 int64_t
 * @{
 */
#define ASSERT_EQ_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INT64(a, b, ...)      ASSERT_TEMPLATE(int64_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_UINT64 uint64_t
 * @{
 */
#define ASSERT_EQ_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINT64(a, b, ...)     ASSERT_TEMPLATE(uint64_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_SIZE size_t
 * @{
 */
#define ASSERT_EQ_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_SIZE(a, b, ...)       ASSERT_TEMPLATE(size_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_PTRDIFF ptrdiff_t
 * @{
 */
#define ASSERT_EQ_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_PTRDIFF(a, b, ...)    ASSERT_TEMPLATE(ptrdiff_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_INTPTR inttpr_t
 * @{
 */
#define ASSERT_EQ_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_INTPTR(a, b, ...)     ASSERT_TEMPLATE(intptr_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @defgroup TEST_ASSERTION_C99_UINTPTR uinttpr_t
 * @{
 */
#define ASSERT_EQ_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_UINTPTR(a, b, ...)    ASSERT_TEMPLATE(uintptr_t, >=, a, b, __VA_ARGS__)
/**
 * @}
 */

/**
 * @}
 */

/**
 * Group: TEST_ASSERTION
 * @}
 */

/**
 * @defgroup TEST_CUSTOM_TYPE Custom type assertion support
 *
 * Even though cutest have rich set of assertion macros, there might be some
 * cases that need to compare custom type.
 *
 * We have a custom type register system to support such scene.
 *
 * Suppose we have a custom type: `typedef struct { int a; } foo_t`, to add
 * type support:
 *
 * + Register type information by #TEST_REGISTER_TYPE_ONCE()
 *
 *   ```c
 *   static int _on_cmp_foo(const foo_t* addr1, const foo_t* addr2) {
 *       return addr1->a - addr2->a;
 *   }
 *   static int _on_dump_foo(FILE* stream, const foo_t* addr) {
 *       return fprintf(stream, "{ a:%d }", addr->a);
 *   }
 *   int main(int argc, char* argv[]) {
 *       TEST_REGISTER_TYPE_ONCE(foo_t, _on_cmp_foo, _on_dump_foo);
 *       return cutest_run_tests(argc, argv, stdout, NULL);
 *   }
 *   ```
 *
 * + Define assertion macros
 *
 *   ```c
 *   #define ASSERT_EQ_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, ==, a, b, __VA_ARGS__)
 *   #define ASSERT_NE_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, !=, a, b, __VA_ARGS__)
 *   #define ASSERT_LT_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, <,  a, b, __VA_ARGS__)
 *   #define ASSERT_LE_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, <=, a, b, __VA_ARGS__)
 *   #define ASSERT_GT_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, >,  a, b, __VA_ARGS__)
 *   #define ASSERT_GE_FOO(a, b, ...)   ASSERT_TEMPLATE(foo_t, >=, a, b, __VA_ARGS__)
 *   ```
 *
 * Now you can use `ASSERT_EQ_FOO()` / `ASSERT_NE_FOO()` / etc to do assertion.
 *
 * @{
 */

/**
 * @brief Declare and register custom type.
 *
 * This function does following things:
 * 1. Try best to check function prototype of \p cmp and \p dump. Note this depends on comiler so might not work.
 * 2. Generate information for \p TYPE.
 * 3. Ensure this register code only run once.
 *
 * For example:
 *
 * + If \p TYPE is `unsigned`, the protocol of \p cmp and \p dump should be:
 *   ```c
 *   int (*)(const unsigned* addr1, const unsigned* addr2) {\
 *       unsigned v1 = *addr1, v2 = *addr2;
 *       if (v1 == v2)
 *           return 0;
 *       return v1 < v2 ? -1 : 1;
 *   }
 *   int (*)(FILE* stream, const unsigned* addr) {
 *       fprintf(stream, "%u", *addr);
 *   }
 *   ```
 *
 * + If \p TYPE is `const char*`, the protocol of \p cmp and \p dump should be:
 *   ```c
 *   int (*)(const char** addr1, const char** addr2) {\
 *       return strcmp(*addr1, *addr2);
 *   }
 *   int (*)(FILE* stream, const char** addr) {
 *       fprintf(stream, "%s", *addr);
 *   }
 *   ```
 *
 * @note Although not restricted, it is recommend to register all custom type before run any test.
 * @param[in] TYPE      Data type.
 * @param[in] fn_cmp    Compare function. It must have proto of `int (*)(const TYPE*, const TYPE*)`.
 * @param[in] fn_dump   Dump function. It must have proto of `int (*)(FILE*, const TYPE*)`.
 */
#define TEST_REGISTER_TYPE_ONCE(TYPE, fn_cmp, fn_dump)    \
    do {\
        /* Try our best to check function protocol. */\
        int (*ckeck_type_cmp)(TYPE*,TYPE*) = fn_cmp; (void)ckeck_type_cmp;\
        int (*check_type_dump)(FILE*, TYPE*) = fn_dump; (void)check_type_dump;\
        /* Register type information. */\
        static cutest_type_info_t s_info = {\
            { NULL, NULL, NULL },\
            #TYPE,\
            (cutest_custom_type_cmp_fn)fn_cmp,\
            (cutest_custom_type_dump_fn)fn_dump,\
        };\
        static int s_token = 0;\
        if (s_token == 0) {\
            s_token = 1;\
            cutest_internal_register_type(&s_info);\
        }\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

/**
 * @brief Compare template.
 * @warning It is for internal usage.
 * @param[in] TYPE  Type name.
 * @param[in] OP    Compare operation.
 * @param[in] a     Left operator.
 * @param[in] b     Right operator.
 * @param[in] fmt   Extra print format when assert failure.
 * @param[in] ...   Print arguments.
 */
#define ASSERT_TEMPLATE(TYPE, OP, a, b, fmt, ...) \
    do {\
        TYPE _L = (a); TYPE _R = (b);\
        if (cutest_internal_compare(#TYPE, (const void*)&_L, (const void*)&_R) OP 0) {\
            break;\
        }\
        cutest_internal_dump(__FILE__, __LINE__, \
            #TYPE, #OP, #a, #b, (const void*)&_L, (const void*)&_R);\
        TEST_INTERNAL_SELECT(TEST_INTERNAL_NONE, cutest_internal_printf, fmt)(fmt, ##__VA_ARGS__);\
        if (cutest_internal_break_on_failure()) {\
            TEST_DEBUGBREAK;\
        }\
        cutest_internal_assert_failure();\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

/** @cond */

#define TEST_INTERNAL_SELECT(a, b, ...)  \
    TEST_JOIN(TEST_INTERNAL_SELECT_, TEST_BARG(__VA_ARGS__))(a, b)

#define TEST_INTERNAL_SELECT_0(a, b) a
#define TEST_INTERNAL_SELECT_1(a, b) b

#define TEST_INTERNAL_NONE(...)

/**
 * @brief Compare function for specific type.
 * @param[in] addr1     Address of value1.
 * @param[in] addr2     Address of value2.
 * @return              0 if equal, <0 if value1 is less than value2, >0 if value1 is more than value2.
 */
typedef int (*cutest_custom_type_cmp_fn)(const void* addr1, const void* addr2);

/**
 * @brief Dump value.
 * @param[in] stream    The file stream to print.
 * @param[in] addr      The address of value.
 * @return              The number of characters printed.
 */
typedef int (*cutest_custom_type_dump_fn)(FILE* stream, const void* addr);

/**
 * @brief Custom type information.
 * @note It is for internal usage.
 */
typedef struct cutest_type_info
{
    cutest_map_node_t           node;       /**< Map node. */
    const char*                 type_name;  /**< The name of type. */
    cutest_custom_type_cmp_fn   cmp;        /**< Compare function. */
    cutest_custom_type_dump_fn  dump;       /**< Dump function. */
} cutest_type_info_t;

/**
 * @brief Register custom type.
 * @warning Use #TEST_REGISTER_TYPE_ONCE().
 * @param[in] info          Type information.
 */
void cutest_internal_register_type(cutest_type_info_t* info);

/**
 * @def TEST_MSVC_WARNNING_GUARD(exp, code)
 * @brief Disable warning for `code'.
 * @note This macro only works for MSVC.
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#   define TEST_MSVC_WARNNING_GUARD(exp, code)  \
        __pragma(warning(push)) \
        __pragma(warning(disable : code)) \
        exp \
        __pragma(warning(pop))
#else
#   define TEST_MSVC_WARNNING_GUARD(exp, code) \
        exp
#endif

/**
 * @def TEST_DEBUGBREAK
 * @brief Causes a breakpoint in your code, where the user will be prompted to
 *   run the debugger.
 */
#if defined(_MSC_VER)
#   define TEST_DEBUGBREAK      __debugbreak()
#elif (defined(__clang__) || defined(__GNUC__)) && (defined(__x86_64__) || defined(__i386__))
#   define TEST_DEBUGBREAK      __asm__ volatile("int $0x03")
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__thumb__)
#   define TEST_DEBUGBREAK      __asm__ volatile(".inst 0xde01")
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__arm__) && !defined(__thumb__)
#   define TEST_DEBUGBREAK      __asm__ volatile(".inst 0xe7f001f0")
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__aarch64__) && defined(__APPLE__)
#   define TEST_DEBUGBREAK      __builtin_debugtrap()
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__aarch64__)
#   define TEST_DEBUGBREAK      __asm__ volatile(".inst 0xd4200000")
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__powerpc__)
#   define TEST_DEBUGBREAK      __asm__ volatile(".4byte 0x7d821008")
#elif (defined(__clang__) || defined(__GNUC__)) && defined(__riscv)
#   define TEST_DEBUGBREAK      __asm__ volatile(".4byte 0x00100073")
#else
#   define TEST_DEBUGBREAK      *(volatile int*)NULL = 1
#endif

/**
 * @brief Compare value1 and value2 of specific type.
 * @param[in] type_name The name of type.
 * @param[in] addr1     The address of value1.
 * @param[in] addr2     The address of value2.
 * @return              Compare result.
 */
int cutest_internal_compare(const char* type_name, const void* addr1, const void* addr2);

/**
 * @brief Dump compare result.
 * @param[in] file      The file name.
 * @param[in] line      The line number.
 * @param[in] type_name The name of type.
 * @param[in] op        The string of operation.
 * @param[in] op_l      The string of left operator.
 * @param[in] op_r      The string of right operator.
 * @param[in] addr1     The address of value1.
 * @param[in] addr2     The address of value2.
 * @param[in] fmt       User defined print format.
 * @param[in] ...       Print arguments to \p fmt.
 */
void cutest_internal_dump(const char* file, int line, const char* type_name,
    const char* op, const char* op_l, const char* op_r,
    const void* addr1, const void* addr2);

void cutest_internal_printf(const char* fmt, ...);

/**
 * @brief Check if `--test_break_on_failure` is set.
 * @return              Boolean.
 */
int cutest_internal_break_on_failure(void);

/**
 * @brief Set current test as failure
 * @note This function is available in setup stage and test body.
 * @warning Call this function in TearDown stage will cause assert.
 */
void cutest_internal_assert_failure(void);

/** @endcond */

/**
 * Group: TEST_CUSTOM_TYPE
 * @}
 */

/**
 * @defgroup TEST_RUN Run
 * @{
 */

/**
 * @brief CUnitTest hook
 */
typedef struct cutest_hook
{
    /**
     * @brief Hook before run all tests
     * @param[in] argc  The number of arguments
     * @param[in] argv  Argument list
     */
    void(*before_all_test)(int argc, char* argv[]);

    /**
     * @brief Hook after run all tests
     */
    void(*after_all_test)(void);

    /**
     * @brief Hook before #TEST_FIXTURE_SETUP() is called
     * @param[in] fixture   Fixture name
     */
    void(*before_setup)(const char* fixture);

    /**
     * @brief Hook after #TEST_FIXTURE_SETUP() is called
     * @param[in] fixture   Fixture name
     * @param[in] ret       zero: #TEST_FIXTURE_SETUP() success, otherwise failure
     */
    void(*after_setup)(const char* fixture, int ret);

    /**
     * @brief Hook before #TEST_FIXTURE_TEARDOWN() is called
     * @param[in] fixture   Fixture name
     */
    void(*before_teardown)(const char* fixture);

    /**
     * @brief Hook after #TEST_FIXTURE_TEARDOWN() is called
     * @param[in] fixture   Fixture name
     * @param[in] ret       zero: #TEST_FIXTURE_TEARDOWN() success, otherwise failure
     */
    void(*after_teardown)(const char* fixture, int ret);

    /**
     * @brief Hook before #TEST_F() is called
     * @param[in] fixture       Fixture name
     * @param[in] test_name     Test name
     */
    void(*before_test)(const char* fixture, const char* test_name);

    /**
     * @brief Hook after #TEST_F() is called
     * @param[in] fixture       Fixture name
     * @param[in] test_name     Test name
     * @param[in] ret           zero: #TEST_F() success, otherwise failure
     */
    void(*after_test)(const char* fixture, const char* test_name, int ret);
} cutest_hook_t;

/**
 * @brief Run all test cases
 * @snippet main.c ENTRYPOINT
 * @param[in] argc      The number of arguments.
 * @param[in] argv      The argument list.
 * @param[in] out       Output stream, cannot be NULL.
 * @param[in] hook      Test hook.
 * @return              0 if success, otherwise failure.
 */
int cutest_run_tests(int argc, char* argv[], FILE* out, const cutest_hook_t* hook);

/**
 * @brief Get current running suit name
 * @return              The suit name
 */
const char* cutest_get_current_fixture(void);

/**
 * @brief Get current running case name
 * @return              The case name
 */
const char* cutest_get_current_test(void);

/**
 * @brief Skip current test case.
 * @note This function only has affect in setup stage.
 * @see TEST_CLASS_SETUP
 */
void cutest_skip_test(void);

/**
 * Group: TEST_RUN
 * @}
 */

/**
 * @defgroup TEST_PORTING Porting
 *
 * By default [cutest](https://github.com/qgymib/cutest) support Windows/Linux,
 * and if you need to porting it to some other OS, you need to implementation
 * all the interfaces listed in this chapter.
 *
 * Do note that the following API is non-optional and operation system must provide it:
 * + [va_start()](https://linux.die.net/man/3/va_start)
 * + [va_end()](https://linux.die.net/man/3/va_end)
 *
 * @{
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API System API
 * 
 * To porting all the interfaces, add `CUTEST_PORTING` (eg. `-DCUTEST_PORTING`)
 * when compile [cutest](https://github.com/qgymib/cutest).
 *
 * To porting specific interface, add one or more flags listed below:
 * | Interface                      | Flag                          |
 * | ------------------------------ | ----------------------------- |
 * | cutest_porting_abort           | CUTEST_PORTING_ABORT          |
 * | cutest_porting_clock_gettime   | CUTEST_PORTING_CLOCK_GETTIME  |
 * | cutest_porting_cvfprintf       | CUTEST_PORTING_CVFPRINTF      |
 * | cutest_porting_gettid          | CUTEST_PORTING_GETTID         |
 * | cutest_porting_setjmp          | CUTEST_PORTING_SETJMP         |
 *
 * @{
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API_ABORT abort()
 * @{
 */

/**
 * @see https://man7.org/linux/man-pages/man3/abort.3.html
 * @note It is not recommend to ignore last words because that will missing
 *   something really important.
 * @param[in] fmt   Last words.
 * @param[in] ap    Arguments to last words.
 */
void cutest_porting_abort(const char* fmt, va_list ap);

/**
 * END GROUP: TEST_PORTING_SYSTEM_API_ABORT
 * @}
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API_CLOCK_GETTIME clock_gettime()
 * @{
 */

typedef struct cutest_porting_timespec
{
    long    tv_sec;
    long    tv_nsec;
} cutest_porting_timespec_t;

/**
 * @see https://linux.die.net/man/3/clock_gettime
 */
void cutest_porting_clock_gettime(cutest_porting_timespec_t* tp);

/**
 * END GROUP: TEST_PORTING_SYSTEM_API_CLOCK_GETTIME
 * @}
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API_FPRINTF fprintf()
 * @{
 */

typedef enum cutest_porting_color
{
    CUTEST_COLOR_DEFAULT,
    CUTEST_COLOR_RED,
    CUTEST_COLOR_GREEN,
    CUTEST_COLOR_YELLOW,
} cutest_porting_color_t;

/**
 * @brief Colorful print.
 *
 * Different system have different solutions to do colorful print, so we have a
 * interface to do it.
 *
 * Below is a simple implementation that works just fine except drop colorful
 * support:
 *
 * ```c
 * int cutest_porting_cvfprintf(FILE* stream, int color, const char* fmt, va_list ap)
 * {
 *     (void)color;
 *     return vfprintf(stream, fmt, ap);
 * }
 * ```
 *
 * @note The color support is optional, it only affects the vision on console.
 *   If you do not known how to add support for it, just ignore the parameter
 *   \p color.
 *
 * @warning If you want to support colorful print, do remember to distinguish
 *   whether data is print to console. If \p stream is a normal file and the
 *   color control charasters are printed into the file, this file is highly
 *   possible mess up.
 *
 * @param[in] stream    The stream to print.
 * @param[in] color     Print color. Checkout #cutest_porting_color_t.
 * @param[in] fmt       Print format.
 * @param[in] ap        Print arguments.
 */
int cutest_porting_cvfprintf(FILE* stream, int color, const char* fmt, va_list ap);

/**
 * END GROUP: TEST_PORTING_SYSTEM_API_FPRINTF
 * @}
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API_GETTID gettid()
 * @{
 */

/**
 * @brief Get current thread ID.
 *
 * This function is used for check whether ASSERTION macros are called from
 * main thread. If your system does not support multithread, just return NULL.
 *
 * @see https://man7.org/linux/man-pages/man3/pthread_self.3.html
 * @see https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentthreadid
 */
void* cutest_porting_gettid(void);

/**
 * END GROUP: TEST_PORTING_SYSTEM_API_GETTID
 * @}
 */

/**
 * @defgroup TEST_PORTING_SYSTEM_API_SETJMP setjmp()
 * @{
 */

typedef struct cutest_porting_jmpbuf cutest_porting_jmpbuf_t;

/**
 * @brief Function protocol for longjmp().
 * @param[in] buf   Jump buffer.
 * @param[in] val   Jump value, cannot be 0.
 */
typedef void (*cutest_porting_longjmp_fn)(cutest_porting_jmpbuf_t* buf, int val);

/**
 * @brief Execute function when call setjmp().
 * @param[in] buf           Jump buffer.
 * @param[in] fn_longjmp    Function to do longjmp.
 * @param[in] val           0 if first call, otherwise is the value passed to \p fn_longjmp.
 * @param[in] data          User defined data passed to #cutest_porting_setjmp()/
 */
typedef void (*cutest_porting_setjmp_fn)(cutest_porting_jmpbuf_t* buf,
    cutest_porting_longjmp_fn fn_longjmp, int val, void* data);

/**
 * @brief Wrapper for setjmp() and longjmp().
 *
 * A example implementation is:
 *
 * ```c
 * struct cutest_porting_jmpbuf
 * {
 *     jmp_buf buf;
 * };
 * static void _cutest_porting_longjmp(cutest_porting_jmpbuf_t* buf, int val)
 * {
 *     longjmp(buf->buf, val);
 * }
 * void cutest_porting_setjmp(cutest_porting_setjmp_fn execute, void* data)
 * {
 *     cutest_porting_jmpbuf_t jmpbuf;
 *
 *     execute(
 *         &jmpbuf,
 *         _cutest_porting_longjmp,
 *         setjmp(jmpbuf.buf),
 *         data);
 * }
 * ```
 *
 * @see https://man7.org/linux/man-pages/man3/setjmp.3.html
 */
void cutest_porting_setjmp(cutest_porting_setjmp_fn execute, void* data);

/**
 * END GROUP: TEST_PORTING_SYSTEM_API_SETJMP
 * @}
 */

/**
 * Group: TEST_PORTING_SYSTEM_API
 * @}
 */

/**
 * @defgroup TEST_FLOATING_NUMBER Floating-Point Numbers
 *
 * By default we use `EPSILON` and `ULP` to do floating number compre, which
 * assume all floating numbers are matching IEEE 754 Standard (both `float` and
 * `double`).
 *
 * This works fine on most time, but there are indeed some hardware does
 * not follow this standard, and this technology just broken.
 *
 * To use custom compare algorithm, define `CUTEST_PORTING_COMPARE_FLOATING_NUMBER`
 * (eg. `-DCUTEST_PORTING_COMPARE_FLOATING_NUMBER`) when compile.
 *
 * @see https://bitbashing.io/comparing-floats.html
 *
 * @{
 */

/**
 * @brief Floating number compare.
 * @param[in] type  The type of \p v1 and \p v2. 0 is float, 1 is double.
 * @param[in] v1    The address to value1.
 * @param[in] v2    The address to value2.
 * @return          <0, =0, >0 if \p v1 is less than, equal to, more than \p v2.
 */
int cutest_porting_compare_floating_number(int type, const void* v1, const void* v2);

/**
 * Group: TEST_FLOATING_NUMBER
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
