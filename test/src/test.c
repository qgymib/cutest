#include "test.h"
#include <stdlib.h>
#include <errno.h>

test_runtime_t _TEST;

void test_register_case(test_case_t* test_case)
{
    if (_TEST.head == NULL)
    {
        _TEST.head = test_case;
        _TEST.tail = test_case;
        return;
    }

    _TEST.tail->next = test_case;
    _TEST.tail = test_case;
}

static void _run_test(test_case_t* test_case)
{
    if (test_case->setup != NULL)
    {
        test_case->setup();
    }

    test_case->body();

    if (test_case->teardown != NULL)
    {
        test_case->teardown();
    }
}

#if !defined(_WIN32)
static int tmpfile_s(FILE** pFilePtr)
{
    if ((*pFilePtr = tmpfile()) != NULL)
    {
        return 0;
    }
    return errno;
}
#endif

static void _close_tmpfile(void)
{
    if (_TEST.out != NULL)
    {
        fclose(_TEST.out);
        _TEST.out = NULL;
    }
}

static void _reset_tmpfile(void)
{
    _close_tmpfile();

    int errcode;
    if ((errcode = tmpfile_s(&_TEST.out)) != 0)
    {
        abort();
    }
}

static void _at_exit(void)
{
    _close_tmpfile();
}

static void _reset_runtime(void)
{
    _reset_tmpfile();
    memset(&_TEST.hook, 0, sizeof(_TEST.hook));
}

int main(int argc, char* argv[])
{
    _TEST.argc = argc;
    _TEST.argv = argv;

    atexit(_at_exit);

    _TEST.cur = _TEST.head;
    for (; _TEST.cur != NULL; _TEST.cur = _TEST.cur->next)
    {
        _reset_runtime();

        fprintf(stdout, "[ RUN      ] %s\n", _TEST.cur->name);
        _run_test(_TEST.cur);
        fprintf(stdout, "[       OK ] %s\n", _TEST.cur->name);
    }

    return 0;
}

void test_print_file(FILE* dst, FILE* src)
{
    char buffer[1024];
    long src_pos = ftell(src);

    fseek(src, 0, SEEK_SET);

    for (;;)
    {
        size_t read_size = fread(buffer, 1, sizeof(buffer), src);
        if (read_size == 0)
        {
            break;
        }

        fwrite(buffer, 1, read_size, dst);
    }
    
    fseek(src, src_pos, SEEK_SET);
}

static int cutest_porting_cprintf_2(FILE* stream, int color, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_porting_cvprintf(stream, color, fmt, ap);
    va_end(ap);

    return ret;
}

void cutest_porting_assert_fail_2(const char* expr, const char* file, int line, const char* func)
{
    cutest_porting_cprintf_2(stderr, CUTEST_COLOR_DEFAULT,
        "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
    cutest_porting_cprintf_2(stderr, CUTEST_COLOR_DEFAULT, "CUTEST OUTPUT:\n");
    test_print_file(stderr, _TEST.out);
    cutest_porting_cprintf_2(stderr, CUTEST_COLOR_DEFAULT, "<< EOF\n");
    cutest_porting_abort("");
}
