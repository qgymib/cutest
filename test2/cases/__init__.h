#ifndef __CUTEST_TEST_CASE_INIT_H__
#define __CUTEST_TEST_CASE_INIT_H__

#include "cutest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_case
{
    /**
     * @brief Test case name.
     */
    const char* name;

    /**
     * @brief Test case body.
     */
    void (*proc)(void);
} test_case_t;

int test_run_all_tests(void);

#ifdef __cplusplus
}
#endif
#endif
