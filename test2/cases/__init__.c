#include <stdlib.h>
#include "__init__.h"

extern const test_case_t cmd_also_run_disabled_tests;

static const test_case_t* s_test_cases[] = {
    &cmd_also_run_disabled_tests,
};

int test_run_all_tests(void)
{
    size_t i;
    for (i = 0; i < TEST_ARRAY_SIZE(s_test_cases); i++)
    {
        const test_case_t* tc = s_test_cases[i];

        fprintf(stdout, "[ RUN      ] %s\n", tc->name);
        tc->proc();
        fprintf(stdout, "[       OK ] %s\n", tc->name);
    }

    return 0;
}
