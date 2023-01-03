#include "test.h"

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

int main(int argc, char* argv[])
{
    _TEST.argc = argc;
    _TEST.argv = argv;

    test_case_t* it = _TEST.head;
    for (; it != NULL; it = it->next)
    {
        _run_test(it);
    }

    return 0;
}
