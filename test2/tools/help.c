#include <stdlib.h>
#include <stdio.h>
#include "__init__.h"

static const char* s_help_str =
"This program contains tests written using cutest. You can use the\n"
"following command line flags to control its behavior:\n"
"\n"
"Test Selection:\n"
"  --help\n"
"      Show what features cutest support.\n"
"\n"
"Tool Selection:\n"
"  --\n"
"      By using `-- [CMD]` (pay attention to the space) you are able to launch\n"
"      builtin tools. Anything followed by `--` will be treat as command\n"
"      arguments to builtin tools. After builtin tools finished, the program\n"
"      will exit.\n"
;

static void _print_help(const char* help, const char* prefix)
{
    size_t i;
    int flag_need_prefix = 1;

    for (i = 0; help[i] != '\0'; i++)
    {
        if (flag_need_prefix)
        {
            flag_need_prefix = 0;
            printf("%s", prefix);
        }

        printf("%c", help[i]);

        if (help[i] == '\n')
        {
            flag_need_prefix = 1;
        }
    }

    printf("\n");
}

static int _print_tools(const test_tool_t* info, void* arg)
{
    (void)arg;

    printf("  -- %s\n", info->cmd);
    if (info->help != NULL)
    {
        _print_help(info->help, "      ");
    }
    return 0;
}

static int _tool_help(int argc, char* argv[])
{
    (void)argc; (void)argv;

    printf("%s", s_help_str);
    test_tool_foreach(_print_tools, NULL);

    fflush(NULL);

    return EXIT_SUCCESS;
}

const test_tool_t test_tool_help = {
    "help", _tool_help,
    "Show this help and exit."
};
