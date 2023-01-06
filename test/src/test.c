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
    if (_TEST.hook.out != NULL)
    {
        fclose(_TEST.hook.out);
        _TEST.hook.out = NULL;
    }
}

static void _reset_tmpfile(void)
{
    _close_tmpfile();

    int errcode;
    if ((errcode = tmpfile_s(&_TEST.hook.out)) != 0)
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
}

int main(int argc, char* argv[])
{
    _TEST.argc = argc;
    _TEST.argv = argv;

    atexit(_at_exit);

    cutest_timestamp_t beg, end, dif;

    test_case_t* it = _TEST.head;
    for (; it != NULL; it = it->next)
    {
        _reset_runtime();
        fprintf(stdout, "[ RUN      ] %s\n", it->name);

        cutest_timestamp_get(&beg);
        _run_test(it);
        cutest_timestamp_get(&end);

        cutest_timestamp_dif(&beg, &end, &dif);
        unsigned long ms = (unsigned long)(dif.sec * 1000 + dif.usec / 1000);

        fprintf(stdout, "[       OK ] %s (%lu ms)\n", it->name, ms);
    }

    return 0;
}
