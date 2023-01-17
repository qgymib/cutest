/**
 * @file
 */
/**
 * @mainpage CUnitTest
 * CUnitTest is a test framework for C. It's was inspired by GoogleTest originally.
 *
 * CUnitTest has following features:
 * + C89 / C99 / C11 compatible.
 * + GCC / Clang / MSVC compatible.
 * + x86 / x86_64 / arm / arm64 compatible.
 * + No dynamic memory alloc at runtime.
 * + Tests are automatically registered when declared.
 * + Support parameterized tests.
 */
#ifndef __C_UNIT_TEST_H__
#define __C_UNIT_TEST_H__

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Major version.
 */
#define CUTEST_VERSION_MAJOR        1

/**
 * @brief Minor version.
 */
#define CUTEST_VERSION_MINOR        0

/**
 * @brief Patch version.
 */
#define CUTEST_VERSION_PATCH        4

/**
 * @brief Development version.
 */
#define CUTEST_VERSION_PREREL       5

/**
 * @brief Ensure the api is exposed as C function.
 */
#if defined(__cplusplus)
#define TEST_C_API  extern "C"
#else
#define TEST_C_API
#endif

/************************************************************************/
/* CUnitTest                                                            */
/************************************************************************/

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
 *     ASSERT_EQ_D32(0, 0);
 * }
 * ```
 *
 * Both #TEST_F() and #TEST_P() define a set of shared test unit, which share
 * the same setup and teardown procedure defined by #TEST_FIXTURE_SETUP() and
 * #TEST_FIXTURE_TEAREDOWN().
 *
 * ```c
 * TEST_FIXTURE_SETUP(foo) {
 *     printf("setup of foo.\n");
 * }
 * TEST_FIXTURE_TEAREDOWN(foo) {
 *     printf("teardown of foo.\n");
 * }
 * 
 * TEST_F(foo, normal) {
 *     ASSERT_NE_D32(0, 0);
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
 */
#define TEST_FIXTURE_SETUP(fixture)    \
    TEST_C_API static void s_cutest_fixture_setup_##fixture(void)

/**
 * @brief TearDown test suit
 * @param [in] fixture  The name of fixture
 */
#define TEST_FIXTURE_TEAREDOWN(fixture)    \
    TEST_C_API static void s_cutest_fixture_teardown_##fixture(void)

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
    TEST_C_API static cutest_parameterized_info_t* s_cutest_parameterized_##fixture##_##test(void){\
        static TYPE s_parameterized_userdata[] = { __VA_ARGS__ };\
        static cutest_case_node_t s_nodes[TEST_ARG_COUNT(__VA_ARGS__)];\
        static cutest_parameterized_info_t s_parameterized_info = {\
            #TYPE, TEST_STRINGIFY(__VA_ARGS__),\
            sizeof(s_parameterized_userdata) / sizeof(s_parameterized_userdata[0]),\
            (void*)s_parameterized_userdata,\
            s_nodes, sizeof(s_nodes) / sizeof(s_nodes[0]),\
        };\
        return &s_parameterized_info;\
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
 * @note If you declare a Parameterized Test but do not want to use #TEST_GET_PARAM(),
 *   you may get a compile time warning like `unused parameter _test_parameterized_data`.
 *   To suppress this warning, just place #TEST_PARAMETERIZED_SUPPRESS_UNUSED
 *   in the begin of your test body.
 *
 * @param [in] fixture  The name of fixture
 * @param [in] test     The name of test case
 * @see TEST_GET_PARAM()
 * @see TEST_PARAMETERIZED_DEFINE()
 * @see TEST_PARAMETERIZED_SUPPRESS_UNUSED
 * @snippet test_p.c
 */
#define TEST_P(fixture, test) \
    TEST_C_API void u_cutest_body_##fixture##_##test(\
        u_cutest_parameterized_type_##fixture##_##test*, size_t);\
    TEST_INITIALIZER(TEST_INIT_##fixture##_##test) {\
        static cutest_case_t _case_##fixture##_##test = {\
            {\
                #fixture,\
                #test,\
            }, /* .info */\
            {\
                s_cutest_fixture_setup_##fixture,\
                s_cutest_fixture_teardown_##fixture,\
                (void(*)(void*, size_t))u_cutest_body_##fixture##_##test,\
            }, /* stage */\
            s_cutest_parameterized_##fixture##_##test, /* parameterized */\
        };\
        cutest_parameterized_info_t* info = s_cutest_parameterized_##fixture##_##test();\
        cutest_register_case(&_case_##fixture##_##test, info->nodes, info->node_sz);\
    }\
    TEST_C_API void u_cutest_body_##fixture##_##test(\
        u_cutest_parameterized_type_##fixture##_##test* _test_parameterized_data,\
        size_t _test_parameterized_idx)

/**
 * @brief Test Fixture
 * @param [in] fixture  The name of fixture
 * @param [in] test     The name of test case
 */
#define TEST_F(fixture, test) \
    TEST_C_API void u_cutest_body_##fixture##_##test(void);\
    TEST_C_API static void s_cutest_proxy_##fixture##_##test(void* _test_parameterized_data,\
        size_t _test_parameterized_idx) {\
        TEST_PARAMETERIZED_SUPPRESS_UNUSED;\
        u_cutest_body_##fixture##_##test();\
    }\
    TEST_INITIALIZER(TEST_INIT_##fixture##_##test) {\
        static cutest_case_t _case_##fixture##_##test = {\
            {\
                #fixture,\
                #test,\
            }, /* .info */\
            {\
                s_cutest_fixture_setup_##fixture,\
                s_cutest_fixture_teardown_##fixture,\
                s_cutest_proxy_##fixture##_##test,\
            }, /* stage */\
            NULL, /* parameterized */\
        };\
        static cutest_case_node_t s_node;\
        cutest_register_case(&_case_##fixture##_##test, &s_node, 1);\
    }\
    TEST_C_API void u_cutest_body_##fixture##_##test(void)

/**
 * @brief Simple Test
 * @param [in] fixture  suit name
 * @param [in] test     case name
 */
#define TEST(fixture, test)  \
    TEST_C_API void u_cutest_body_##fixture##_##test(void);\
    TEST_C_API static void s_cutest_proxy_##fixture##_##test(void* _test_parameterized_data,\
        size_t _test_parameterized_idx) {\
        TEST_PARAMETERIZED_SUPPRESS_UNUSED;\
        u_cutest_body_##fixture##_##test();\
    }\
    TEST_INITIALIZER(TEST_INIT_##fixture##_##test) {\
        static cutest_case_t _case_##fixture##_##test = {\
            {\
                #fixture,\
                #test,\
            }, /* .info */\
            {\
                NULL, NULL,\
                s_cutest_proxy_##fixture##_##test,\
            }, /* stage */\
            NULL, /* parameterized */\
        };\
        static cutest_case_node_t s_node;\
        cutest_register_case(&_case_##fixture##_##test, &s_node, 1);\
    }\
    TEST_C_API void u_cutest_body_##fixture##_##test(void)

/** @cond */

/**
 * @def TEST_INITIALIZER(f)
 * @brief Run the following code before main() invoke.
 */
#ifdef __cplusplus
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
        TEST_C_API void f(void) __attribute__((constructor)); \
        TEST_C_API void f(void)
#else
#   error "INITIALIZER not support on your compiler"
#endif

/**
 * @def TEST_ARG_COUNT
 * @brief Get the number of arguments
 */
#ifdef _MSC_VER // Microsoft compilers
#   define TEST_ARG_COUNT(...)  \
        TEST_INTERNAL_EXPAND_ARGS_PRIVATE(TEST_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define TEST_INTERNAL_ARGS_AUGMENTER(...)    \
        unused, __VA_ARGS__
#   define TEST_INTERNAL_EXPAND_ARGS_PRIVATE(...)   \
        TEST_EXPAND(TEST_INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define TEST_INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#else // Non-Microsoft compilers
#   define TEST_ARG_COUNT(...)  \
        TEST_INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define TEST_INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#endif

#define TEST_STRINGIFY(...)     TEST_STRINGIFY_2(__VA_ARGS__)
#define TEST_STRINGIFY_2(...)   #__VA_ARGS__

#define TEST_EXPAND(x)          x
#define TEST_JOIN(a, b)         TEST_JOIN2(a, b)
#define TEST_JOIN2(a, b)        a##b

typedef struct cutest_case_node cutest_case_node_t;

typedef struct cutest_map_node
{
    struct cutest_map_node*     __rb_parent_color;      /**< father node | color */
    struct cutest_map_node*     rb_right;               /**< right child node */
    struct cutest_map_node*     rb_left;                /**< left child node */
} cutest_map_node_t;

typedef struct cutest_parameterized_info
{
    const char*                 type_name;              /**< User type name. */

    const char*                 test_data_info;         /**< The C string of user test data. */
    size_t                      test_data_sz;           /**< parameterized data size */
    void*                       test_data;              /**< parameterized data */

    cutest_case_node_t*         nodes;
    size_t                      node_sz;
} cutest_parameterized_info_t;

typedef struct cutest_case
{
    struct
    {
        const char*             fixture;                /**< suit name */
        const char*             case_name;              /**< case name */
    } info;

    struct
    {
        void                    (*setup)(void);         /**< setup */
        void                    (*teardown)(void);      /**< teardown */
        void                    (*body)(void*, size_t); /**< test body */
    } stage;

    cutest_parameterized_info_t* (*get_parameterized_info)(void);
} cutest_case_t;

struct cutest_case_node
{
    cutest_map_node_t           node;
    cutest_case_t*              test_case;
    unsigned                    mask;                   /**< internal mask */
    unsigned long               randkey;
    size_t                      parameterized_idx;
};

/**
 * @brief Register test case
 * @param[in] test_case     Test case
 */
void cutest_register_case(cutest_case_t* test_case, cutest_case_node_t* node, size_t node_sz);

 /** @endcond */

/**
 * Group: TEST_DEFINE
 * @}
 */

/**
 * @defgroup TEST_ASSERTION Assertion
 * 
 * [cutest](https://github.com/qgymib/cutest/) support rich set of assertion. An assertion typically have following syntax:
 * 
 * ```c
 * ASSERT_OP_TYPE(a, b)
 * ASSERT_OP_TYPE(a, b, fmt, ...)
 * ```
 * 
 * The `OP` means which compare operation you want to use:
 * + EQ: `a` is equal to `b`.
 * + NE: `a` is not equal to `b`.
 * + LT: `a` is less than `b`.
 * + LE: `a` is equal to `b` or less than `b`.
 * + GT: `a` is greater than `b`.
 * + GE: `a` is equal to `b` or greater than `b`.
 *
 * The `TYPE` means the type of value `a` and `b`.
 * + D32: int32_t
 * + U32: uint32_t
 * + D64: int64_t
 * + U64: uint64_t
 * + STR: const char*
 * + PTR: const void*
 * + SIZE: size_t
 * + FLOAT: float
 * + DOUBLE: double
 *
 * > To support more types, checkout #TEST_REGISTER_TYPE().
 *
 * So, an assertion like #ASSERT_EQ_D32() means except `a` and `b` have type of `int32_t` and they are the same value.
 *
 * You may notice all assertions have syntax of `ASSERT_OP_TYPE(a, b, fmt, ...)`, it means custom print is available if assertion fails. For example, the following code
 * 
 * ```c
 * int errcode = ENOENT;
 * ASSERT_EQ_D32(0, errcode, "%s(%d)", strerror(errcode), errcode);
 * ```
 *
 * Will print something like:
 *
 * ```
 * No such file or directory(2)
 * ```
 *
 * You may also want to refer to the actual value of operator, you can use `_L` to refer to left operator and `_R` to refer to right operator:
 * 
 * ```c
 * ASSERT_EQ_D32(0, 1+2, "%d is not %d", _L, _R);
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
 * @def ASSERT_EQ_D32
 * @brief Assert `a' == `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_D32
 * @brief Assert `a' != `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_D32
 * @brief Assert `a' < `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_D32
 * @brief Assert `a' <= `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_D32
 * @brief Assert `a' > `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_D32
 * @brief Assert `a' >= `b'. `a' and `b' must has type `int32_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_D32(a, b, ...)        ASSERT_TEMPLATE_EXT(int32_t, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_U32
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_U32
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_U32
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_U32
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_U32
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_U32
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint32_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_U32(a, b, ...)        ASSERT_TEMPLATE_EXT(uint32_t, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_D64
 * @brief Assert `a' == `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_D64
 * @brief Assert `a' != `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_D64
 * @brief Assert `a' < `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_D64
 * @brief Assert `a' <= `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_D64
 * @brief Assert `a' > `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_D64
 * @brief Assert `a' >= `b'. `a' and `b' must has type `int64_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_D64(a, b, ...)        ASSERT_TEMPLATE_EXT(int64_t, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_U64
 * @brief Assert `a' == `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_U64
 * @brief Assert `a' != `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_U64
 * @brief Assert `a' < `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_U64
 * @brief Assert `a' <= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_U64
 * @brief Assert `a' > `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_U64
 * @brief Assert `a' >= `b'. `a' and `b' must has type `uint64_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_U64(a, b, ...)        ASSERT_TEMPLATE_EXT(uint64_t, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_SIZE
 * @brief Assert `a' == `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_SIZE
 * @brief Assert `a' != `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_SIZE
 * @brief Assert `a' < `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_SIZE
 * @brief Assert `a' <= `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_SIZE
 * @brief Assert `a' > `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_SIZE
 * @brief Assert `a' >= `b'. `a' and `b' must has type `size_t'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_SIZE(a, b, ...)       ASSERT_TEMPLATE_EXT(size_t, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_PTR
 * @brief Assert `a' == `b'. `a' and `b' must be pointer type.
 *
 * If `a' != `b' does not match, this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_PTR
 * @brief Assert `a' != `b'. `a' and `b' must be pointer type.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_PTR
 * @brief Assert `a' < `b'. `a' and `b' must be pointer type.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_PTR
 * @brief Assert `a' <= `b'. `a' and `b' must be pointer type.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_PTR
 * @brief Assert `a' > `b'. `a' and `b' must be pointer type.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_PTR
 * @brief Assert `a' >= `b'. `a' and `b' must be pointer type.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_PTR(a, b, ...)        ASSERT_TEMPLATE_EXT(const void*, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_FLOAT
 * @brief Assert `a' == `b'. `a' and `b' must has type `float'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_FLOAT
 * @brief Assert `a' != `b'. `a' and `b' must has type `float'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LT_FLOAT
 * @brief Assert `a' < `b'. `a' and `b' must has type `float'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_FLOAT
 * @brief Assert `a' <= `b'. `a' and `b' must has type `float'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_FLOAT
 * @brief Assert `a' > `b'. `a' and `b' must has type `float'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_FLOAT
 * @brief Assert `a' >= `b'. `a' and `b' must has type `float'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_EXT(float, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_DOUBLE
 * @brief Assert `a' == `b'. `a' and `b' must has type `double'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_NE_DOUBLE
 * @brief Assert `a' != `b'. `a' and `b' must has type `double'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a        Value a
 * @param [in] b        Value b
 * @param [in] ...      User defined error message
 */
/**
 * @def ASSERT_LT_DOUBLE
 * @brief Assert `a' < `b'. `a' and `b' must has type `double'.
 *
 * If `a' >= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_LE_DOUBLE
 * @brief Assert `a' <= `b'. `a' and `b' must has type `double'.
 *
 * If `a' > `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GT_DOUBLE
 * @brief Assert `a' > `b'. `a' and `b' must has type `double'.
 *
 * If `a' <= `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
/**
 * @def ASSERT_GE_DOUBLE
 * @brief Assert `a' >= `b'. `a' and `b' must has type `double'.
 *
 * If `a' < `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a    Value a
 * @param [in] b    Value b
 * @param [in] ...  User defined error message
 */
#define ASSERT_EQ_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, !=, a, b, __VA_ARGS__)
#define ASSERT_LT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, <,  a, b, __VA_ARGS__)
#define ASSERT_LE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, <=, a, b, __VA_ARGS__)
#define ASSERT_GT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, >,  a, b, __VA_ARGS__)
#define ASSERT_GE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_EXT(double, >=, a, b, __VA_ARGS__)

/**
 * @def ASSERT_EQ_STR
 * @brief Assert `a' == `b'. `a' and `b' must has type `const char*'.
 *
 * If `a' != `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a        Value a
 * @param [in] b        Value b
 * @param [in] ...      User defined error message
 */
/**
 * @def ASSERT_NE_STR
 * @brief Assert `a' != `b'. `a' and `b' must has type `const char*'.
 *
 * If `a' == `b', this test will stop immediately and mark as failure.
 *
 * @param [in] a        Value a
 * @param [in] b        Value b
 * @param [in] ...      User defined error message
 */
#define ASSERT_EQ_STR(a, b, ...)        ASSERT_TEMPLATE_EXT(const char*, ==, a, b, __VA_ARGS__)
#define ASSERT_NE_STR(a, b, ...)        ASSERT_TEMPLATE_EXT(const char*, !=, a, b, __VA_ARGS__)

/**
 * Group: TEST_ASSERTION
 * @}
 */

/**
 * @defgroup TEST_CUSTOM_TYPE Custom type
 * 
 * Even though cutest have rich set of assertion macros, there might be some cases that need to compare custom type.
 * 
 * We have a custom type register system to support such scene.
 *
 * Suppose we have a custom type: `typedef struct { int a; } foo_t`, to add type support:
 *
 * + Register type information by #TEST_REGISTER_TYPE()
 *
 *   ```c
 *   static int _on_cmp_foo(const foo_t* addr1, const foo_t* addr2) {
 *       return addr1->a - addr2->a;
 *   }
 *   static int _on_dump_foo(FILE* file, const foo_t* addr) {
 *       return fprintf(file, "{ a:%d }", addr->a);
 *   }
 *   TEST_REGISTER_TYPE(foo_t, _on_cmp_foo, _on_dump_foo)
 *   ```
 * 
 * + Define assertion macros
 * 
 *   ```c
 *   #define ASSERT_EQ_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, ==, a, b, __VA_ARGS__)
 *   #define ASSERT_NE_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, !=, a, b, __VA_ARGS__)
 *   #define ASSERT_LT_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, <,  a, b, __VA_ARGS__)
 *   #define ASSERT_LE_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, <=, a, b, __VA_ARGS__)
 *   #define ASSERT_GT_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, >,  a, b, __VA_ARGS__)
 *   #define ASSERT_GE_FOO(a, b, ...)   ASSERT_TEMPLATE_EXT(foo_t, >=, a, b, __VA_ARGS__)
 *   ```
 * 
 * Now you can use `ASSERT_EQ_FOO()` / `ASSERT_NE_FOO()` / etc to do assertion.
 * 
 * @{
 */

/**
 * @brief Declare and register custom type.
 * @param[in] TYPE  Data type.
 * @param[in] cmp   Compare function. It must have proto of `int (*)(const TYPE*, const TYPE*)`.
 * @param[in] dump  Dump function
 */
#define TEST_REGISTER_TYPE(TYPE, cmp, dump)    \
    TEST_INITIALIZER(TEST_USER_TYPE_##TYPE) {\
        /* Try our best to check function protocol. */\
        int (*ckeck_type_cmp)(const TYPE*,const TYPE*) = cmp; (void)ckeck_type_cmp;\
        int (*check_type_dump)(FILE*, const TYPE*) = dump; (void)check_type_dump;\
        /* Register type information. */\
        static cutest_type_info_t info = {\
            { NULL, NULL, NULL },\
            #TYPE,\
            (cutest_custom_type_cmp_fn)cmp,\
            (cutest_custom_type_dump_fn)dump,\
        };\
        cutest_register_type(&info);\
    }

 /** @cond */

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
#define ASSERT_TEMPLATE_EXT(TYPE, OP, a, b, fmt, ...) \
    do {\
        TYPE _L = (a); TYPE _R = (b);\
        if (cutest_internal_compare(#TYPE, (void*)&_L, (void*)&_R) OP 0) {\
            break;\
        }\
        cutest_internal_dump(__FILE__, __LINE__, #TYPE, #OP, #a, #b, (void*)&_L, (void*)&_R,\
            "" fmt, ##__VA_ARGS__);\
        if (cutest_internal_break_on_failure()) {\
            TEST_DEBUGBREAK;\
        }\
        cutest_internal_assert_failure();\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

/**
 * @def TEST_MSVC_WARNNING_GUARD(exp, code)
 * @brief Disable warning for `code'.
 * @note This macro only works for MSVC.
 */
#if defined(_MSC_VER)
#   define TEST_MSVC_WARNNING_GUARD(exp, code)  \
        __pragma(warning(push))\
        __pragma(warning(disable : code))\
        exp\
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
 * @brief Compare function for specific type.
 * @param[in] addr1     Address of value1.
 * @param[in] addr2     Address of value2.
 * @return              0 if equal, <0 if value1 is less than value2, >0 if value1 is more than value2.
 */
typedef int (*cutest_custom_type_cmp_fn)(const void* addr1, const void* addr2);

/**
 * @brief Dump value.
 * @param[in] file      The file to print.
 * @param[in] addr      The address of value.
 * @return              The number of characters printed.
 */
typedef int (*cutest_custom_type_dump_fn)(FILE* file, const void* addr);

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
 * @note It is for internal usage. Use #TEST_REGISTER_TYPE() instead.
 */
void cutest_register_type(cutest_type_info_t* info);

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
    const void* addr1, const void* addr2, const char* fmt, ...);

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

void cutest_unwrap_assert_fail(const char* expr, const char* file, int line, const char* func);
int cutest_printf(const char* fmt, ...);
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
     * @brief Where the output should be written.
     *
     * If set to NULL, `stdout` is used.
     */
    FILE*   out;

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
     * @brief Hook before #TEST_FIXTURE_TEAREDOWN() is called
     * @param[in] fixture   Fixture name
     */
    void(*before_teardown)(const char* fixture);

    /**
     * @brief Hook after #TEST_FIXTURE_TEAREDOWN() is called
     * @param[in] fixture   Fixture name
     * @param[in] ret       zero: #TEST_FIXTURE_TEAREDOWN() success, otherwise failure
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
 * @param[in] argc      The number of arguments
 * @param[in] argv      The argument list
 * @param[in] hook      Test hook
 * @return              The number of failure test
 */
int cutest_run_tests(int argc, char* argv[], const cutest_hook_t* hook);

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
 * @defgroup TEST_TIMESTAMP Timestamp
 * @{
 */

/**
 * @brief The timestamp
 */
typedef struct cutest_timestamp
{
    uint64_t    sec;        /**< seconds */
    uint64_t    usec;       /**< microseconds */
}cutest_timestamp_t;

/**
 * @brief Monotonic time since some unspecified starting point
 * @param[out] t    Timestamp.
 */
void cutest_timestamp_get(cutest_timestamp_t* t);

/**
 * @brief Compare timestamp
 * @param [in] t1       timestamp t1
 * @param [in] t2       timestamp t2
 * @param [in] dif      diff
 * @return              -1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
int cutest_timestamp_dif(const cutest_timestamp_t* t1, const cutest_timestamp_t* t2, cutest_timestamp_t* dif);

/**
 * Group: TEST_TIMESTAMP
 * @}
 */

#ifdef __cplusplus
}
#endif
#endif
