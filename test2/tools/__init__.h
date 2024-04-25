#ifndef __CUTEST_TEST_TOOL_INIT_H__
#define __CUTEST_TEST_TOOL_INIT_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_tool
{
    const char* cmd;
    int (*proc)(int argc, char* argv[]);
    const char* help;
} test_tool_t;

int test_tool_exec(int argc, char* argv[]);
void test_tool_foreach(int (*cb)(const test_tool_t*, void*), void* arg);

#ifdef __cplusplus
}
#endif
#endif
