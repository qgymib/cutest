#ifndef __TEST_H__
#define __TEST_H__

#undef NDEBUG
#include <assert.h>
#include "cutest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_case_s
{
    struct test_case_s* next;

    void (*setup)(void);
    void (*teardown)(void);
    void (*body)(void);
} test_case_t;

typedef struct test_runtime_s
{
    test_case_t*    head;
    test_case_t*    tail;

    int             argc;
    char**          argv;

    int             rret;   /**< Run result. */
} test_runtime_t;

extern test_runtime_t _TEST;

#define DEFINE_SETUP(fixture)  \
    void test_setup_##fixture(void)

#define DEFINE_TEARDOWN(fixture)   \
    void test_teardown_##fixture(void)

#define DEFINE_TEST_F(fixture, name, ...) \
    void test_body_##fixture##_##name(void);\
    DEFINE_TEST_ENTRY(test_entry_##fixture##_##name, test_body_##fixture##_##name, ##__VA_ARGS__)\
    TEST_INITIALIZER(test_##name) {\
        static test_case_t test_case = {\
            NULL,\
            test_setup_##fixture,\
            test_teardown_##fixture,\
            test_entry_##fixture##_##name,\
        };\
        test_register_case(&test_case);\
    }\
    void test_body_##fixture##_##name(void)

#define DEFINE_TEST(fixture, name, ...)   \
    void test_body_##fixture##_##name(void);\
    DEFINE_TEST_ENTRY(test_entry_##fixture##_##name, test_body_##fixture##_##name, ##__VA_ARGS__)\
    TEST_INITIALIZER(test_##name) {\
        static test_case_t test_case = {\
            NULL, NULL, NULL,\
            test_entry_##fixture##_##name,\
        };\
        test_register_case(&test_case);\
    }\
    void test_body_##fixture##_##name(void)

#define DEFINE_TEST_ENTRY(NAME, fn, ...)   \
    void NAME(void) {\
        char* argv[] = {\
            _TEST.argv[0],\
            ##__VA_ARGS__,\
            NULL\
        };\
        int argc = sizeof(argv) / sizeof(argv[0]) - 1;\
        _TEST.rret = cutest_run_tests(argc, argv, NULL);\
        fn();\
    }

void test_register_case(test_case_t* test_case);

#ifdef __cplusplus
}
#endif

#endif
