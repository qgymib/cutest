#ifndef __TEST_H__
#define __TEST_H__

#undef NDEBUG
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cutest.h"
#include "string_matrix.h"

#define DEFINE_SETUP(fixture)  \
    void test_setup_##fixture(void)

#define DEFINE_TEARDOWN(fixture)   \
    void test_teardown_##fixture(void)

#define DEFINE_TEST_F(fixture, name, ...) \
    void test_body_##fixture##_##name(void);\
    DEFINE_TEST_ENTRY(test_entry_##fixture##_##name, test_body_##fixture##_##name, ##__VA_ARGS__)\
    TEST_INITIALIZER(test_##name) {\
        static test_case_t test_case = {\
            NULL, #fixture "." #name,\
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
            NULL, #fixture "." #name,\
            NULL, NULL,\
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
        _TEST.rret = cutest_run_tests(argc, argv, &_TEST.hook);\
        fn();\
    }

#define ASSERT_STRING_EQ(s1, s2)   \
    do {\
        const char* _s1 = (s1);\
        const char* _s2 = (s2);\
        if (strcmp(s1, s2) != 0) {\
            fprintf(stderr, "%s:%d:failure:\n"\
                "            expected: `%s` vs `%s`\n"\
                "              actual: `%s`\n"\
                "                  vs: `%s`\n",\
                __FILE__, __LINE__, #s1, #s2, _s1, _s2);\
            abort();\
        }\
    } while (0);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_case_s
{
    struct test_case_s* next;
    const char* name;

    void (*setup)(void);
    void (*teardown)(void);
    void (*body)(void);
} test_case_t;

typedef struct test_runtime_s
{
    test_case_t*        head;
    test_case_t*        tail;

    int                 argc;
    char**              argv;

    cutest_hook_t       hook;
    int                 rret;       /**< Run result. */
} test_runtime_t;

extern test_runtime_t _TEST;

void test_register_case(test_case_t* test_case);

#ifdef __cplusplus
}
#endif

#endif
