#include <string.h>
#include <stdlib.h>
#include "__init__.h"
#include "test.h"

extern const test_tool_t test_tool_eolcheck;
extern const test_tool_t test_tool_help;

static const test_tool_t* const g_command_table[] = {
    &test_tool_eolcheck,
    &test_tool_help,
};

int test_tool_exec(int argc, char* argv[])
{
    size_t i;
    for (i = 0; i < TEST_ARRAY_SIZE(g_command_table); i++)
    {
        if (strcmp(argv[0], g_command_table[i]->cmd) != 0)
        {
            continue;
        }

        return g_command_table[i]->proc(argc, argv);
    }

    fprintf(stderr, "%s: command not found\n", argv[0]);
    fflush(NULL);

    return EXIT_FAILURE;
}

void test_tool_foreach(int (*cb)(const test_tool_t*, void*), void* arg)
{
    size_t i;
    for (i = 0; i < TEST_ARRAY_SIZE(g_command_table); i++)
    {
        if (cb(g_command_table[i], arg))
        {
            return;
        }
    }
}
