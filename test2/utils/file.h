#ifndef __CUTEST_TEST_UTILS_FILE_H__
#define __CUTEST_TEST_UTILS_FILE_H__

#include "str.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read file content.
 * @param[in] path - File path.
 * @param[out] data - Buffer to store file content.
 * @return - 0: success
 * @return - -errno: on error
 */
int test_file_read(const char* path, test_str_t* data);

#ifdef __cplusplus
}
#endif
#endif
