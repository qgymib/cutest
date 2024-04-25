#include <stdlib.h>
#include <string.h>
#include "tools/__init__.h"
#include "test.h"

static const char* s_tool_help =
"missing argument after `--`, use `-- help` (pay attention to the space) to see\n"
"what tools builtin.\n"
;

static void _before_all_test(int argc, char* argv[])
{
    int i;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--") == 0)
        {
            if (i == argc - 1)
            {
                fprintf(stderr, "%s", s_tool_help);
                exit(EXIT_FAILURE);
            }

            exit(test_tool_exec(argc - i - 1, argv + i + 1));
        }
    }
}

const cutest_hook_t test_hook = {
    _before_all_test,   /* .before_all_test */
    NULL,               /* .after_all_test */
    NULL,               /* .before_setup */
    NULL,               /* .after_setup */
    NULL,               /* .before_teardown */
    NULL,               /* .after_teardown */
    NULL,               /* .before_test */
    NULL,               /* .after_test */
};
