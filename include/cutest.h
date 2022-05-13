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
#include <stdarg.h>

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
#define CUTEST_VERSION_PATCH        2

/**
 * @brief Development version.
 */
#define CUTEST_VERSION_PREREL       3

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* CUnitTest                                                            */
/************************************************************************/

/**
 * @defgroup CUnitTest CUnitTest
 * @{
 */

/**
 * @defgroup SetupAndTeardown Setup And Teardown
 * @{
 */

/**
 * @brief Setup test fixture
 * @note `fixture_name' must be globally unique
 * @param [in] fixture_name     The name of fixture
 */
#define TEST_FIXTURE_SETUP(fixture_name)    \
    static void TEST_FIXTURE_SETUP_##fixture_name(void)

/**
 * @brief TearDown test suit
 * @note `fixture_name' must be globally unique
 * @param [in] fixture_name     The name of fixture
 */
#define TEST_FIXTURE_TEAREDOWN(fixture_name)    \
    static void TEST_FIXTURE_TEARDOWN_##fixture_name(void)

/**
 * Group: SetupAndTeardown
 * @}
 */

/**
 * @defgroup DefineTest Define Test
 * @{
 */

/**
 * @brief Get parameterized data
 * @return  The data you defined
 */
#define TEST_GET_PARAM()    \
    (_test_parameterized_data[cutest_internal_parameterized_index()])

/**
 * @brief Define parameterized data for fixture
 * @param [in] fixture_name     Which fixture you want to parameterized
 * @param [in] TYPE             Data type
 * @param [in] ...              Data values
 */
#define TEST_PARAMETERIZED_DEFINE(fixture_name, case_name, TYPE, ...)  \
    typedef TYPE _parameterized_type_##fixture_name##_##case_name;\
    static const char* _parameterized_get_user_type_name_##fixture_name##_##case_name(void) {\
        return #TYPE;\
    }\
    static const char* _parameterized_get_builtin_type_name_##fixture_name##_##case_name(void) {\
        TEST_RETURN_BUILTIN_TYPENAME(TYPE);\
    }\
    static unsigned _parameterized_get_type_size_##fixture_name##_##case_name(void) {\
        return sizeof(TYPE);\
    }\
    static TYPE _parameterized_data_##fixture_name##_##case_name[] = { __VA_ARGS__ }

/**
 * @brief Suppress unused parameter warning if #TEST_GET_PARAM() is not used.
 */
#define TEST_PARAMETERIZED_SUPPRESS_UNUSED  \
    (void)_test_parameterized_data

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
 *   To suppress this warning, just place `TEST_PARAMETERIZED_SUPPRESS_UNUSED`
 *   in the begin of your test body.
 *
 * @param [in] fixture_name     The name of fixture
 * @param [in] case_name        The name of test case
 * @see TEST_GET_PARAM()
 * @see TEST_PARAMETERIZED_DEFINE()
 * @see TEST_PARAMETERIZED_SUPPRESS_UNUSED
 */
#define TEST_P(fixture_name, case_name) \
    void TEST_BODY_##fixture_name##_##case_name(_parameterized_type_##fixture_name##_##case_name*);\
    TEST_INITIALIZER(TEST_INIT_##fixture_name##_##case_name) {\
        static cutest_case_t _case_##fixture_name##_##case_name = {\
            { { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
            {\
                CUTEST_CASE_TYPE_PARAMETERIZED,\
                0,\
                #fixture_name,\
                #case_name,\
                #fixture_name "." #case_name,\
            }, /* .info */\
            {\
                TEST_FIXTURE_SETUP_##fixture_name,\
                TEST_FIXTURE_TEARDOWN_##fixture_name,\
                (void*)TEST_BODY_##fixture_name##_##case_name,\
            }, /* stage */\
            {\
                sizeof(_parameterized_data_##fixture_name##_##case_name)\
                    / sizeof(_parameterized_data_##fixture_name##_##case_name[0]), \
                _parameterized_data_##fixture_name##_##case_name, \
                _parameterized_get_user_type_name_##fixture_name##_##case_name,\
                _parameterized_get_builtin_type_name_##fixture_name##_##case_name,\
                _parameterized_get_type_size_##fixture_name##_##case_name,\
            }, /* parameterized */\
        };\
        TEST_REGISTER_TEST_CASE(&_case_##fixture_name##_##case_name);\
    }\
    void TEST_BODY_##fixture_name##_##case_name(\
        _parameterized_type_##fixture_name##_##case_name* _test_parameterized_data)

/**
 * @brief Test Fixture
 * @param [in] fixture_name     The name of fixture
 * @param [in] case_name        The name of test case
 */
#define TEST_F(fixture_name, case_name) \
    void TEST_BODY_##fixture_name##_##case_name(void);\
    TEST_INITIALIZER(TEST_INIT_##fixture_name##_##case_name) {\
        static cutest_case_t _case_##fixture_name##_##case_name = {\
            { { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
            {\
                CUTEST_CASE_TYPE_FIXTURE,\
                0,\
                #fixture_name,\
                #case_name,\
                #fixture_name "." #case_name,\
            }, /* .info */\
            {\
                TEST_FIXTURE_SETUP_##fixture_name,\
                TEST_FIXTURE_TEARDOWN_##fixture_name,\
                (void*)TEST_BODY_##fixture_name##_##case_name,\
            }, /* stage */\
            {\
                0, NULL, NULL, NULL, 0\
            }, /* parameterized */\
        };\
        TEST_REGISTER_TEST_CASE(&_case_##fixture_name##_##case_name);\
    }\
    void TEST_BODY_##fixture_name##_##case_name(void)

/**
 * @brief Simple Test
 * @param [in] suit_name        suit name
 * @param [in] case_name        case name
 */
#define TEST(suit_name, case_name)  \
    void TEST_BODY_##suit_name##_##case_name(void);\
    TEST_INITIALIZER(TEST_INIT_##suit_name##_##case_name) {\
        static cutest_case_t _case_##suit_name##_##case_name = {\
            { { NULL, NULL }, { NULL, NULL, NULL } }, /* .node */\
            {\
                CUTEST_CASE_TYPE_SIMPLE,\
                0,\
                #suit_name,\
                #case_name,\
                #suit_name "." #case_name,\
            }, /* .info */\
            {\
                NULL, NULL, (void*)TEST_BODY_##suit_name##_##case_name,\
            }, /* stage */\
            {\
                0, NULL, NULL,NULL,  0\
            }, /* parameterized */\
        };\
        TEST_REGISTER_TEST_CASE(&_case_##suit_name##_##case_name);\
    }\
    void TEST_BODY_##suit_name##_##case_name(void)

/**
 * Group: DefineTest
 * @}
 */

/**
 * @defgroup Assertion Assertion
 * @{
 */

/**
 * @brief Prints an error message to standard error and terminates the program
 *   by calling abort(3) if expression is false.
 * @note `NDEBUG' has no affect on this macro.
 * @param [in] x        expression
 */
#define ASSERT(x)   \
    do {\
        if (x) {\
            break;\
        }\
        if (cutest_internal_break_on_failure()) {\
            TEST_DEBUGBREAK;\
        }\
        cutest_unwrap_assert_fail(#x, __FILE__, __LINE__, __FUNCTION__);\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

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
#define ASSERT_EQ_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int32_t, "%" PRId32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U32(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint32_t, "%" PRIu32, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_D64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(int64_t, "%" PRId64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_U64(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(uint64_t, "%" PRIu64, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_SIZE(a, b, ...)       ASSERT_TEMPLATE_VA(__VA_ARGS__)(size_t, "%" TEST_PRIsize, >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", ==, _ASSERT_INTERNAL_HELPER_EQ, a, b, ##__VA_ARGS__)
#define ASSERT_NE_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", !=, _ASSERT_INTERNAL_HELPER_NE, a, b, ##__VA_ARGS__)
#define ASSERT_LT_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <,  _ASSERT_INTERNAL_HELPER_LT, a, b, ##__VA_ARGS__)
#define ASSERT_LE_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", <=, _ASSERT_INTERNAL_HELPER_LE, a, b, ##__VA_ARGS__)
#define ASSERT_GT_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >,  _ASSERT_INTERNAL_HELPER_GT, a, b, ##__VA_ARGS__)
#define ASSERT_GE_PTR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const void*, "%p", >=, _ASSERT_INTERNAL_HELPER_GE, a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", ==, cutest_internal_assert_helper_float_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", !=, !cutest_internal_assert_helper_float_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <,  !cutest_internal_assert_helper_float_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", <=, cutest_internal_assert_helper_float_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >,  !cutest_internal_assert_helper_float_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_FLOAT(a, b, ...)      ASSERT_TEMPLATE_VA(__VA_ARGS__)(float, "%f", >=, cutest_internal_assert_helper_float_ge,  a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", ==, cutest_internal_assert_helper_double_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", !=, !cutest_internal_assert_helper_double_eq, a, b, ##__VA_ARGS__)
#define ASSERT_LT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <,  !cutest_internal_assert_helper_double_ge, a, b, ##__VA_ARGS__)
#define ASSERT_LE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", <=, cutest_internal_assert_helper_double_le,  a, b, ##__VA_ARGS__)
#define ASSERT_GT_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >,  !cutest_internal_assert_helper_double_le, a, b, ##__VA_ARGS__)
#define ASSERT_GE_DOUBLE(a, b, ...)     ASSERT_TEMPLATE_VA(__VA_ARGS__)(double, "%f", >=, cutest_internal_assert_helper_double_ge,  a, b, ##__VA_ARGS__)

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
#define ASSERT_EQ_STR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", ==, cutest_internal_assert_helper_str_eq,  a, b, ##__VA_ARGS__)
#define ASSERT_NE_STR(a, b, ...)        ASSERT_TEMPLATE_VA(__VA_ARGS__)(const char*, "%s", !=, !cutest_internal_assert_helper_str_eq, a, b, ##__VA_ARGS__)

/**
 * Group: Assertion
 * @}
 */

/************************************************************************/
/* log                                                                  */
/************************************************************************/

/**
 * @defgroup Log Log
 * @{
 */

typedef enum cutest_log_level
{
    CUTEST_LOG_DEBUG,
    CUTEST_LOG_INFO,
    CUTEST_LOG_WARN,
    CUTEST_LOG_ERROR,
    CUTEST_LOG_FATAL,
} cutest_log_level_t;

typedef enum cutest_print_color
{
    CUTEST_PRINT_COLOR_DEFAULT  = 0,
    CUTEST_PRINT_COLOR_RED      = 1,
    CUTEST_PRINT_COLOR_GREEN    = 2,
    CUTEST_PRINT_COLOR_YELLOW   = 4,
} cutest_print_color_t;

typedef struct cutest_log_meta
{
    cutest_log_level_t  leve;
    const char*         file;
    const char*         func;
    int                 line;
} cutest_log_meta_t;

/**
 * @brief Debug log
 * @param [in] fmt      Print format
 * @param [in] ...      Print arguments
 */
#define TEST_LOG_D(fmt, ...)  \
    do {\
        cutest_log_meta_t info = {\
            CUTEST_LOG_DEBUG,\
            __FILE__,\
            __FUNCTION__,\
            __LINE__,\
        };\
        cutest_log(&info, fmt, ##__VA_ARGS__);\
    } while (0)

#define TEST_LOG_I(fmt, ...)    \
    do {\
        cutest_log_meta_t info = {\
            CUTEST_LOG_INFO,\
            __FILE__,\
            __FUNCTION__,\
            __LINE__,\
        };\
        cutest_log(&info, fmt, ##__VA_ARGS__);\
    } while (0)

#define TEST_LOG_W(fmt, ...)    \
    do {\
        cutest_log_meta_t info = {\
            CUTEST_LOG_WARN,\
            __FILE__,\
            __FUNCTION__,\
            __LINE__,\
        };\
        cutest_log(&info, fmt, ##__VA_ARGS__);\
    } while (0)

#define TEST_LOG_E(fmt, ...)    \
    do {\
        cutest_log_meta_t info = {\
            CUTEST_LOG_ERROR,\
            __FILE__,\
            __FUNCTION__,\
            __LINE__,\
        };\
        cutest_log(&info, fmt, ##__VA_ARGS__);\
    } while (0)

#define TEST_LOG_F(fmt, ...)    \
    do {\
        cutest_log_meta_t info = {\
            CUTEST_LOG_FATAL,\
            __FILE__,\
            __FUNCTION__,\
            __LINE__,\
        };\
        cutest_log(&info, fmt, ##__VA_ARGS__);\
    } while (0)

/**
 * @}
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
     * @param[in] fixture_name  Fixture name
     */
    void(*before_fixture_setup)(const char* fixture_name);

    /**
     * @brief Hook after #TEST_FIXTURE_SETUP() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] ret           zero: #TEST_FIXTURE_SETUP() success, otherwise failure
     */
    void(*after_fixture_setup)(const char* fixture_name, int ret);

    /**
     * @brief Hook before #TEST_FIXTURE_TEAREDOWN() is called
     * @param[in] fixture_name  Fixture name
     */
    void(*before_fixture_teardown)(const char* fixture_name);

    /**
     * @brief Hook after #TEST_FIXTURE_TEAREDOWN() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] ret           zero: #TEST_FIXTURE_TEAREDOWN() success, otherwise failure
     */
    void(*after_fixture_teardown)(const char* fixture_name, int ret);

    /**
     * @brief Hook before #TEST_F() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] test_name     Test name
     */
    void(*before_fixture_test)(const char* fixture_name, const char* test_name);

    /**
     * @brief Hook after #TEST_F() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] test_name     Test name
     * @param[in] ret           zero: #TEST_F() success, otherwise failure
     */
    void(*after_fixture_test)(const char* fixture_name, const char* test_name, int ret);

    /**
     * @brief Hook before #TEST_P() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] test_name     Test name
     * @param[in] index         Current parameterized data index
     * @param[in] total         Amount of parameterized data
     */
    void(*before_parameterized_test)(const char* fixture_name, const char* test_name, unsigned index, unsigned total);

    /**
     * @brief Hook after #TEST_P() is called
     * @param[in] fixture_name  Fixture name
     * @param[in] test_name     Test name
     * @param[in] index         Current parameterized data index
     * @param[in] total         Amount of parameterized data
     * @param[in] ret           zero: #TEST_P() success, otherwise failure
     */
    void(*after_parameterized_test)(const char* fixture_name, const char* test_name, unsigned index, unsigned total, int ret);

    /**
     * @brief Hook before #TEST() is called
     * @param[in] suit_name     Suit name
     * @param[in] test_name     Test name
     */
    void(*before_simple_test)(const char* suit_name, const char* test_name);

    /**
     * @brief Hook after #TEST() is called
     * @param[in] suit_name     Suit name
     * @param[in] test_name     Test name
     * @param[in] ret           zero: #TEST() success, otherwise failure
     */
    void(*after_simple_test)(const char* suit_name, const char* test_name, int ret);

    /**
     * @brief Log output
     * @param[in] info          Log information.
     * @param[in] fmt           Print format.
     * @param[in] ap            Print argument list.
     * @param[in] out           The file to write log.
     */
    void(*on_log_print)(cutest_log_meta_t* info, const char* fmt, va_list ap, FILE* out);
} cutest_hook_t;

/**
 * @brief Run all test cases
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
const char* cutest_get_current_suit_name(void);

/**
 * @brief Get current running case name
 * @return              The case name
 */
const char* cutest_get_current_case_name(void);

/**
 * @brief Skip current test case.
 * @note This function only has affect in setup stage.
 * @see TEST_CLASS_SETUP
 */
void cutest_skip_test(void);

/**
 * Group: CUnitTest
 * @}
 */

/************************************************************************/
/* time                                                                 */
/************************************************************************/

/**
 * @defgroup Timestamp Timestamp
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
 * @param [in] ts       etest_timestamp_t*
 * @return              0 if success, otherwise failure
 */
int cutest_timestamp_get(cutest_timestamp_t* ts);

/**
 * @brief Compare timestamp
 * @param [in] t1       timestamp t1
 * @param [in] t2       timestamp t2
 * @param [in] dif      diff
 * @return              -1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
int cutest_timestamp_dif(const cutest_timestamp_t* t1, const cutest_timestamp_t* t2, cutest_timestamp_t* dif);

/**
 * @}
 */

/************************************************************************/
/* internal interface                                                   */
/************************************************************************/

/** @cond */

/**
 * @internal
 * @def TEST_NOINLINE
 * @brief Prevents a function from being considered for inlining.
 * @note If the function does not have side-effects, there are optimizations
 *   other than inlining that causes function calls to be optimized away,
 *   although the function call is live.
 */
/**
 * @internal
 * @def TEST_NORETURN
 * @brief Define function that never return.
 */
/**
 * @internal
 * @def TEST_UNREACHABLE
 * @brief If control flow reaches the point of the TEST_UNREACHABLE, the
 *   program is undefined.
 */
#if defined(__GNUC__) || defined(__clang__)
#   define TEST_NOINLINE    __attribute__((noinline))
#   define TEST_NORETURN    __attribute__((noreturn))
#   define TEST_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#   define TEST_NOINLINE    __declspec(noinline)
#   define TEST_NORETURN    __declspec(noreturn)
#   define TEST_UNREACHABLE __assume(0)
#else
#   define TEST_NOINLINE
#   define TEST_NORETURN
#   define TEST_UNREACHABLE
#endif

/**
 * @internal
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
 * @internal
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
 * @internal
 * @def TEST_PRIsize
 * @brief A correct format for print `size_t'
 */
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#   define TEST_PRIsize "Iu"
#else
#   define TEST_PRIsize "zu"
#endif

/**
 * @internal
 * @def TEST_ARG_COUNT
 * @brief Get the number of arguments
 */
#ifdef _MSC_VER // Microsoft compilers
#   define TEST_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND(x) x
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#else // Non-Microsoft compilers
#   define TEST_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, count, ...) count
#endif

#define TEST_EXPAND(x)      x
#define TEST_JOIN(a, b)     TEST_JOIN2(a, b)
#define TEST_JOIN2(a, b)    a##b

/**
 * @internal
 * @def TEST_INITIALIZER(f)
 * @brief Run the following code before main() invoke.
 */
#ifdef __cplusplus
#   define TEST_INITIALIZER(f) \
        void f(void); \
        struct f##_t_ { f##_t_(void) { f(); } }; f##_t_ f##_; \
        void f(void)
#elif defined(_MSC_VER)
#   pragma section(".CRT$XCU",read)
#   define TEST_INITIALIZER2_(f,p) \
        void f(void); \
        __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
        __pragma(comment(linker,"/include:" p #f "_")) \
        void f(void)
#   ifdef _WIN64
#       define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"")
#   else
#       define TEST_INITIALIZER(f) TEST_INITIALIZER2_(f,"_")
#   endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__llvm__)
#   define TEST_INITIALIZER(f) \
        void f(void) __attribute__((constructor)); \
        void f(void)
#else
#   error "INITIALIZER not support on your compiler"
#endif

/**
 * @internal
 * @brief   A user defined assert template
 * @param[in] TYPE  Data type
 * @param[in] FMT   Print format
 * @param[in] OP    Compare operation
 * @param[in] CMP   Compare function
 * @param[in] a     Value a
 * @param[in] b     Value b
 * @param[in] u_fmt User defined error message
 * @param[in] ...   User defined error message parameters
 */
#define ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, u_fmt, ...)   \
    do {\
        TYPE _1 = (TYPE)(a); TYPE _2 = (TYPE)(b);\
        if (CMP(_1, _2)) {\
            break;\
        }\
        cutest_printf("%s:%d:failure:" u_fmt "\n"\
            "            expected:    `%s' %s `%s'\n"\
            "              actual:    " FMT " vs " FMT "\n",\
            __FILE__, __LINE__, ##__VA_ARGS__, #a, #OP, #b, _1, _2);\
        cutest_internal_flush();\
        if (cutest_internal_break_on_failure()) {\
            TEST_DEBUGBREAK;\
        } else {\
            cutest_internal_assert_failure();\
        }\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

/**
 * @brief Register test case
 */
#define TEST_REGISTER_TEST_CASE(p_case)\
    do {\
        cutest_case_t* _p_case = p_case;\
        if (cutest_register_case(_p_case) != 0) {\
            fprintf(stderr, "[%s:%d] register %s failed: duplicate name.\n",\
                cutest_pretty_file(__FILE__),\
                __LINE__,\
                _p_case->info.full_name);\
            cutest_internal_flush();\
            TEST_DEBUGBREAK;\
        }\
    } TEST_MSVC_WARNNING_GUARD(while (0), 4127)

#define ASSERT_TEMPLATE_VA(...)                                 TEST_JOIN(ASSERT_TEMPLATE_VA_, TEST_ARG_COUNT(__VA_ARGS__))
#define ASSERT_TEMPLATE_VA_0(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_1(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_2(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_3(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_4(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_5(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_6(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_7(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_8(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))
#define ASSERT_TEMPLATE_VA_9(TYPE, FMT, OP, CMP, a, b, ...)     TEST_EXPAND(ASSERT_TEMPLATE(TYPE, FMT, OP, CMP, a, b, __VA_ARGS__))

#define _ASSERT_INTERNAL_HELPER_EQ(a, b)        ((a) == (b))
#define _ASSERT_INTERNAL_HELPER_NE(a, b)        ((a) != (b))
#define _ASSERT_INTERNAL_HELPER_LT(a, b)        ((a) < (b))
#define _ASSERT_INTERNAL_HELPER_LE(a, b)        ((a) <= (b))
#define _ASSERT_INTERNAL_HELPER_GT(a, b)        ((a) > (b))
#define _ASSERT_INTERNAL_HELPER_GE(a, b)        ((a) >= (b))

#if __STDC__==1 && __STDC_VERSION >= 201112L
#   define TEST_BUILTIN_TYPENAME(x) _Generic((x),                                       \
        char:                   "char", \
        unsigned char:          "unsigned char",\
        signed char:            "signed char",\
        short int:              "short int",\
        unsigned short int:     "unsigned short int",\
        int:                    "int",\
        unsigned int:           "unsigned int",\
        long int:               "long int",\
        unsigned long int:      "unsigned long int",\
        long long int:          "long long int",\
        unsigned long long int: "unsigned long long int",\
        float:                  "float",\
        double:                 "double",\
        long double:            "long double",\
        char* :                 "char*",\
        void* :                 "void*",\
        default:                NULL)
#   define TEST_RETURN_BUILTIN_TYPENAME(x)  return TEST_BUILTIN_TYPENAME(x)
#elif defined(__GNUC__) || defined(__clang__)
#   define TEST_BUILTIN_TYPENAME(x) \
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), char),                   "char",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), unsigned char),          "unsigned char",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), signed char),            "signed char",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), short int),              "short int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), unsigned short int),     "unsigned short int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), int),                    "int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), unsigned int),           "unsigned int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), long int),               "long int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), unsigned long int),      "unsigned long int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), long long int),          "long long int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), unsigned long long int), "unsigned long long int",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), float),                  "float",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), double),                 "double",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), long double),            "long double",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), char*),                  "char*",\
        __builtin_choose_expr(__builtin_types_compatible_p(typeof (x), void*),                  "void*",\
                                                                                                NULL   ))))))))))))))))
#   define TEST_RETURN_BUILTIN_TYPENAME(x)  return TEST_BUILTIN_TYPENAME(x)
#elif defined(_MSC_VER) && defined(__cplusplus)
#   define TEST_RETURN_BUILTIN_TYPENAME(x)\
        static std::string info = typeid(x).name();\
        return info.c_str()
#else
#   define TEST_BUILTIN_TYPENAME(x) NULL
#   define TEST_RETURN_BUILTIN_TYPENAME(x)  return TEST_BUILTIN_TYPENAME(x)
#endif

typedef enum cutest_case_type
{
    CUTEST_CASE_TYPE_SIMPLE,
    CUTEST_CASE_TYPE_FIXTURE,
    CUTEST_CASE_TYPE_PARAMETERIZED,
}cutest_case_type_t;

/**
 * @brief List node
 */
typedef struct cutest_list_node
{
    struct cutest_list_node* p_after;               /**< next node */
    struct cutest_list_node* p_before;              /**< previous node */
}cutest_list_node_t;

/**
* @brief List handler
*/
typedef struct cunittest_list
{
    cutest_list_node_t*     head;                   /**< Head node */
    cutest_list_node_t*     tail;                   /**< Tail node */
    size_t                  size;                   /**< Amount of nodes */
}cutest_list_t;

/**
 * @brief Map node
 */
typedef struct cutest_map_node
{
    struct cutest_map_node* __rb_parent_color;  /**< father node | color */
    struct cutest_map_node* rb_right;           /**< right child node */
    struct cutest_map_node* rb_left;            /**< left child node */
}cutest_map_node_t;

/**
 * @brief Compare function
 * @param [in] key1     KEY1
 * @param [in] key2     KEY2
 * @param [in] arg      User defined argument
 * @return              Compare result
 */
typedef int(*cutest_map_cmp_fn)(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg);

/**
 * @brief Map handler
 */
typedef struct cunittest_map
{
    cutest_map_node_t*      rb_root;    /**< Root node */

    struct
    {
        cutest_map_cmp_fn   cmp;        /**< Compare function */
        void*               arg;        /**< User defined data for compare function */
    }cmp;

    size_t                  size;       /**< Data size */
}cutest_map_t;

typedef void(*cutest_procedure_fn)(void);
typedef void(*cutest_parameterized_fn)(void*);

typedef struct cutest_case
{
    struct
    {
        cutest_list_node_t       queue;                  /**< list node */
        cutest_map_node_t        table;                  /**< map node */
    }node;

    struct
    {
        cutest_case_type_t      type;                   /**< case type */
        unsigned                mask;                   /**< internal mask */
        const char*             suit_name;              /**< suit name */
        const char*             case_name;              /**< case name */
        const char*             full_name;              /**< full name */
    }info;

    struct
    {
        cutest_procedure_fn     setup;                  /**< setup */
        cutest_procedure_fn     teardown;               /**< teardown */
        void*                   body;                   /**< test body */
    }stage;

    struct
    {
        size_t                  n_dat;                  /**< parameterized data size */
        void*                   p_dat;                  /**< parameterized data */
        const char*             (*fn_get_user_type_name)(void);
        const char*             (*fn_get_builtin_type_name)(void);
        unsigned                (*fn_get_type_size)(void);
    }parameterized;
}cutest_case_t;

/**
 * @brief Register test case
 * @param[in] test_case     Test case
 * @return                  0 for success, -1 for failure.
 */
int cutest_register_case(cutest_case_t* test_case);

/**
 * @internal
 * @brief Set current test as failure
 * @note This function is available in setup stage and test body.
 * @warning Call this function in TearDown stage will cause assert.
 */
TEST_NORETURN void cutest_internal_assert_failure(void);
TEST_NORETURN void cutest_unwrap_assert_fail(const char *expr, const char *file, int line, const char *func);
void cutest_internal_flush(void);
int cutest_internal_break_on_failure(void);
int cutest_internal_assert_helper_str_eq(const char* a, const char* b);
int cutest_internal_assert_helper_float_eq(float a, float b);
int cutest_internal_assert_helper_float_le(float a, float b);
int cutest_internal_assert_helper_float_ge(float a, float b);
int cutest_internal_assert_helper_double_eq(double a, double b);
int cutest_internal_assert_helper_double_le(double a, double b);
int cutest_internal_assert_helper_double_ge(double a, double b);
unsigned cutest_internal_parameterized_index(void);
int cutest_printf(const char* fmt, ...);
int cutest_color_fprintf(cutest_print_color_t color, FILE* stream, const char* fmt, ...);
int cutest_color_vfprintf(cutest_print_color_t color, FILE* stream, const char* fmt, va_list ap);
const char* cutest_pretty_file(const char* file);

void cutest_log(cutest_log_meta_t* info, const char* fmt, ...);

/**
 * @brief Map initializer helper
 * @param [in] fn       Compare function
 * @param [in] arg      User defined argument
 */
#define CUTEST_MAP_INIT(fn, arg)      { NULL, { fn, arg }, 0 }
cutest_map_node_t* cutest_map_begin(const cutest_map_t* handler);
cutest_map_node_t* cutest_map_next(const cutest_map_node_t* node);
int cutest_map_insert(cutest_map_t* handler, cutest_map_node_t* node);

cutest_list_node_t* cutest_list_begin(const cutest_list_t* handler);
cutest_list_node_t* cutest_list_next(const cutest_list_node_t* node);
size_t cutest_list_size(const cutest_list_t* handler);
void cutest_list_erase(cutest_list_t* handler, cutest_list_node_t* node);
void cutest_list_push_back(cutest_list_t* handler, cutest_list_node_t* node);

typedef enum cutest_optparse_argtype
{
    CUTEST_OPTPARSE_NONE,
    CUTEST_OPTPARSE_REQUIRED,
    CUTEST_OPTPARSE_OPTIONAL,
} cutest_optparse_argtype_t;

typedef struct cutest_optparse_long_opt
{
    const char*                 longname;
    int                         shortname;
    cutest_optparse_argtype_t   argtype;
} cutest_optparse_long_opt_t;

typedef struct cutest_optparse
{
    char**                      argv;
    int                         permute;
    int                         optind;
    int                         optopt;
    char*                       optarg;
    char                        errmsg[64];
    size_t                      subopt;
} cutest_optparse_t;

void cutest_optparse_init(cutest_optparse_t *options, char **argv);
int cutest_optparse_long(cutest_optparse_t *options,
        const cutest_optparse_long_opt_t *longopts, int *longindex);

/** @endcond */

#ifdef __cplusplus
}
#endif
#endif
