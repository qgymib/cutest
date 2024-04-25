#ifndef __CUTEST_TEST_UTILS_STR_H__
#define __CUTEST_TEST_UTILS_STR_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_str
{
    char*   str;
    size_t  len;
    size_t  cap;
} test_str_t;

#define TEST_STR_INIT   { NULL, 0, 0 }

/**
 * @brief Destroy string.
 * @param[in,out] str - String
 */
void test_str_exit(test_str_t* str);

/**
 * @brief Append \p data to \p str.
 * @param[in,out] str - String.
 * @param[in] data - Data to append.
 * @param[in] size - Length of \p data.
 */
void test_str_append(test_str_t* str, const char* data, size_t size);

/**
 * @brief Duplicate string.
 * @param[in] str - String to duplicate.
 * @return - Duplicated string.
 */
test_str_t test_str_dup(const test_str_t* str);

#ifdef __cplusplus
}
#endif
#endif
