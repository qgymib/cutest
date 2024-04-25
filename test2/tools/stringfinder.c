#include "__init__.h"

static int _string_finder(int argc, char* argv[])
{

}

const test_tool_t test_tool_stringfinder = {
"stringfinder", _string_finder,
"Find string in file.\n"
};
