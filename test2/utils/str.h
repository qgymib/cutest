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

void test_str_exit(test_str_t* str);
void test_str_append(test_str_t* str, const char* data, size_t len);

#ifdef __cplusplus
}
#endif
#endif
