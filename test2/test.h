#ifndef __CUTEST_TEST_H__
#define __CUTEST_TEST_H__

#include "cutest.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Global test hook.
 */
extern const cutest_hook_t test_hook;

void cutest_test_run_tools(int argc, char* argv[]);

#ifdef __cplusplus
}
#endif
#endif
